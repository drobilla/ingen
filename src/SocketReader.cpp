/*
  This file is part of Ingen.
  Copyright 2007-2017 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ingen/SocketReader.hpp"

#include "ingen/AtomForge.hpp"
#include "ingen/AtomReader.hpp"
#include "ingen/Log.hpp"
#include "ingen/URIMap.hpp"
#include "ingen/World.hpp"
#include "lv2/urid/urid.h"
#include "raul/Socket.hpp"
#include "sord/sordmm.hpp"

#include <cerrno>
#include <cstdint>
#include <memory>
#include <mutex>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <utility>

namespace ingen {

SocketReader::SocketReader(ingen::World&                 world,
                           Interface&                    iface,
                           std::shared_ptr<raul::Socket> sock)
    : _world(world)
    , _iface(iface)
    , _env()
    , _inserter(nullptr)
    , _msg_node(nullptr)
    , _socket(std::move(sock))
    , _socket_error(0)
    , _exit_flag(false)
    , _thread(&SocketReader::run, this)
{}

SocketReader::~SocketReader()
{
	_exit_flag = true;
	_socket->shutdown();
	_thread.join();
}

SerdStatus
SocketReader::set_base_uri(SocketReader*   iface,
                           const SerdNode* uri_node)
{
	return sord_inserter_set_base_uri(iface->_inserter, uri_node);
}

SerdStatus
SocketReader::set_prefix(SocketReader*   iface,
                         const SerdNode* name,
                         const SerdNode* uri_node)
{
	return sord_inserter_set_prefix(iface->_inserter, name, uri_node);
}

SerdStatus
SocketReader::write_statement(SocketReader*      iface,
                              SerdStatementFlags flags,
                              const SerdNode*    graph,
                              const SerdNode*    subject,
                              const SerdNode*    predicate,
                              const SerdNode*    object,
                              const SerdNode*    object_datatype,
                              const SerdNode*    object_lang)
{
	if (!iface->_msg_node) {
		iface->_msg_node = sord_node_from_serd_node(
			iface->_world.rdf_world()->c_obj(), iface->_env, subject, nullptr, nullptr);
	}

	return sord_inserter_write_statement(
		iface->_inserter, flags, graph,
		subject, predicate, object,
		object_datatype, object_lang);
}

size_t
SocketReader::c_recv(void* buf, size_t size, size_t nmemb, void* stream)
{
	auto* self = static_cast<SocketReader*>(stream);

	const ssize_t c = recv(self->_socket->fd(), buf, size * nmemb, MSG_WAITALL);
	if (c < 0) {
		self->_socket_error = errno;
		return 0;
	}

	return c;
}

int
SocketReader::c_err(void* stream)
{
	auto* self = static_cast<SocketReader*>(stream);

	return self->_socket_error;
}

void
SocketReader::run()
{
	Sord::World*  world = _world.rdf_world();
	LV2_URID_Map& map   = _world.uri_map().urid_map();

	// Set up a forge to build LV2 atoms from model
	SordNode*  base_uri = nullptr;
	SordModel* model    = nullptr;
	AtomForge  forge(map);
	{
		// Lock RDF world
		std::lock_guard<std::mutex> lock(_world.rdf_mutex());

		// Use <ingen:/> as base URI, so relative URIs are like bundle paths
		base_uri = sord_new_uri(world->c_obj(),
		                        reinterpret_cast<const uint8_t*>("ingen:/"));

		// Make a model and reader to parse the next Turtle message
		_env = world->prefixes().c_obj();
		model = sord_new(world->c_obj(), SORD_SPO, false);

		// Create an inserter for writing incoming triples to model
		_inserter = sord_inserter_new(model, _env);
	}

	SerdReader* reader = serd_reader_new(
		SERD_TURTLE, this, nullptr,
		reinterpret_cast<SerdBaseSink>(set_base_uri),
		reinterpret_cast<SerdPrefixSink>(set_prefix),
		reinterpret_cast<SerdStatementSink>(write_statement),
		nullptr);

	serd_env_set_base_uri(_env, sord_node_to_serd_node(base_uri));
	serd_reader_start_source_stream(reader,
	                                c_recv,
	                                c_err,
	                                this,
	                                reinterpret_cast<const uint8_t*>(
	                                    "(socket)"),
	                                1);

	// Make an AtomReader to call Ingen Interface methods based on Atom
	AtomReader ar(_world.uri_map(), _world.uris(), _world.log(), _iface);

	struct pollfd pfd{};
	pfd.fd      = _socket->fd();
	pfd.events  = POLLIN|POLLPRI;
	pfd.revents = 0;

	while (!_exit_flag && !_socket_error) {
		// Wait for input to arrive at socket
		const int ret = poll(&pfd, 1, -1);
		if (ret == -1 || (pfd.revents & (POLLERR|POLLHUP|POLLNVAL))) {
			on_hangup();
			break;  // Hangup
		} else if (!ret) {
			continue;  // No data, shouldn't happen
		}

		// Lock RDF world
		std::lock_guard<std::mutex> lock(_world.rdf_mutex());

		// Read until the next '.'
		SerdStatus st = serd_reader_read_chunk(reader);
		if (st == SERD_FAILURE || !_msg_node) {
			continue;  // Read nothing, e.g. just whitespace
		} else if (st) {
			_world.log().error("Read error: %1%\n", serd_strerror(st));
			continue;
		}

		// Build an LV2_Atom at chunk.buf from the message
		forge.read(*world, model, _msg_node);

		// Call _iface methods based on atom content
		ar.write(forge.atom());

		// Reset everything for the next iteration
		forge.clear();
		sord_node_free(world->c_obj(), _msg_node);
		_msg_node = nullptr;
	}

	// Lock RDF world
	std::lock_guard<std::mutex> lock(_world.rdf_mutex());

	// Destroy everything
	sord_inserter_free(_inserter);
	serd_reader_end_stream(reader);
	serd_reader_free(reader);
	sord_free(model);
	_socket.reset();
}

}  // namespace ingen
