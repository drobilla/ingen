/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <errno.h>
#include <poll.h>

#include "ingen/AtomReader.hpp"
#include "ingen/Interface.hpp"
#include "ingen/Log.hpp"
#include "ingen/SocketReader.hpp"
#include "ingen/URIMap.hpp"
#include "ingen/World.hpp"
#include "sord/sordmm.hpp"
#include "sratom/sratom.h"

namespace Ingen {

SocketReader::SocketReader(Ingen::World&      world,
                           Interface&         iface,
                           SPtr<Raul::Socket> sock)
	: _world(world)
	, _iface(iface)
	, _inserter(NULL)
	, _msg_node(NULL)
	, _socket(sock)
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
			iface->_world.rdf_world()->c_obj(), iface->_env, subject, 0, 0);
	}

	return sord_inserter_write_statement(
		iface->_inserter, flags, graph,
		subject, predicate, object,
		object_datatype, object_lang);
}

void
SocketReader::run()
{
	Sord::World*  world = _world.rdf_world();
	LV2_URID_Map* map   = &_world.uri_map().urid_map_feature()->urid_map;

	// Open socket as a FILE for reading directly with serd
	FILE* f = fdopen(_socket->fd(), "r");
	if (!f) {
		_world.log().error(fmt("Failed to open connection (%1%)\n")
		                   % strerror(errno));
		// Connection gone, exit
		_socket.reset();
		return;
	}

	// Use <ingen:/root/> as base URI so e.g. </foo/bar> will be a path
	SordNode* base_uri = sord_new_uri(
		world->c_obj(), (const uint8_t*)"ingen:/root/");

	// Set up sratom and a forge to build LV2 atoms from model
	Sratom*        sratom = sratom_new(map);
	SerdChunk      chunk  = { NULL, 0 };
	LV2_Atom_Forge forge;
	lv2_atom_forge_init(&forge, map);
	lv2_atom_forge_set_sink(
		&forge, sratom_forge_sink, sratom_forge_deref, &chunk);

	// Make a model and reader to parse the next Turtle message
	_env = world->prefixes().c_obj();
	SordModel* model = sord_new(world->c_obj(), SORD_SPO, false);

	_inserter = sord_inserter_new(model, _env);

	SerdReader* reader = serd_reader_new(
		SERD_TURTLE, this, NULL,
		(SerdBaseSink)set_base_uri,
		(SerdPrefixSink)set_prefix,
		(SerdStatementSink)write_statement,
		NULL);

	serd_env_set_base_uri(_env, sord_node_to_serd_node(base_uri));
	serd_reader_start_stream(reader, f, (const uint8_t*)"(socket)", false);

	// Make an AtomReader to call Ingen Interface methods based on Atom
	AtomReader ar(_world.uri_map(),
	              _world.uris(),
	              _world.log(),
	              _world.forge(),
	              _iface);

	struct pollfd pfd;
	pfd.fd      = _socket->fd();
	pfd.events  = POLLIN;
	pfd.revents = 0;

	while (!_exit_flag) {
		if (feof(f)) {
			break;  // Lost connection
		}

		// Wait for input to arrive at socket
		int ret = poll(&pfd, 1, -1);
		if (ret == -1 || (pfd.revents & (POLLERR|POLLHUP|POLLNVAL))) {
			break;  // Hangup
		} else if (!ret) {
			continue;  // No data, shouldn't happen
		}

		// Read until the next '.'
		SerdStatus st = serd_reader_read_chunk(reader);
		if (st == SERD_FAILURE || !_msg_node) {
			continue;  // Read nothing, e.g. just whitespace
		} else if (st) {
			_world.log().error(fmt("Read error: %1%\n")
			                   % serd_strerror(st));
			continue;
		}

		// Build an LV2_Atom at chunk.buf from the message
		sratom_read(sratom, &forge, world->c_obj(), model, _msg_node);

		// Call _iface methods based on atom content
		ar.write((const LV2_Atom*)chunk.buf);

		// Reset everything for the next iteration
		chunk.len = 0;
		sord_node_free(world->c_obj(), _msg_node);
		_msg_node = NULL;
	}

	fclose(f);
	sord_inserter_free(_inserter);
	serd_reader_end_stream(reader);
	sratom_free(sratom);
	serd_reader_free(reader);
	sord_free(model);
	free((uint8_t*)chunk.buf);
	_socket.reset();
}

}  // namespace Ingen
