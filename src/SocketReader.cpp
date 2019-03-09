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
#include "ingen/types.hpp"
#include "lv2/atom/forge.h"
#include "lv2/urid/urid.h"
#include "raul/Socket.hpp"
#include "serd/serd.hpp"

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <poll.h>
#include <utility>

namespace ingen {

SocketReader::SocketReader(ingen::World&      world,
                           Interface&         iface,
                           SPtr<Raul::Socket> sock)
	: _world(world)
	, _iface(iface)
	, _socket(std::move(sock))
	, _exit_flag(false)
	, _thread(&SocketReader::run, this)
{}

SocketReader::~SocketReader()
{
	_exit_flag = true;
	_socket->shutdown();
	_thread.join();
}

void
SocketReader::run()
{
	serd::World&  world = _world.rdf_world();
	LV2_URID_Map& map   = _world.uri_map().urid_map_feature()->urid_map;

	// Open socket as a FILE for reading directly with serd
	std::unique_ptr<FILE, decltype(&fclose)> f{fdopen(_socket->fd(), "r"),
	                                           &fclose};
	if (!f) {
		_world.log().error("Failed to open connection (%1%)\n",
		                   strerror(errno));
		// Connection gone, exit
		_socket.reset();
		return;
	}

	// Set up a forge to build LV2 atoms from model
	AtomForge                   forge(world, map);
	serd::Optional<serd::Node>  base_uri;
	serd::Optional<serd::Model> model;
	serd::Env                   env;
	UPtr<serd::Inserter>        inserter;
	serd::Optional<serd::Node>  msg_node;
	{
		// Lock RDF world
		std::lock_guard<std::mutex> lock(_world.rdf_mutex());

		// Use <ingen:/> as base URI, so relative URIs are like bundle paths
		base_uri = serd::make_uri("ingen:/");

		// Make a model and reader to parse the next Turtle message
		env   = _world.env();
		model = serd::Model(world, serd::ModelFlag::index_SPO);

		// Create an inserter for writing incoming triples to model
		inserter = UPtr<serd::Inserter>{new serd::Inserter(*model, env)};
	}

	serd::Sink sink;

	sink.set_base_func([&](const serd::Node& uri) {
		return inserter->sink().base(uri);
	});

	sink.set_prefix_func([&](const serd::Node& name, const serd::Node& uri) {
		return inserter->sink().prefix(name, uri);
	});

	sink.set_statement_func([&](const serd::StatementFlags flags,
	                            const serd::Statement&     statement) {
		if (!msg_node) {
			msg_node = statement.subject();
		}

		return inserter->sink().statement(flags, statement);
	});

	serd::Reader reader(world, serd::Syntax::Turtle, {}, sink, 4096);

	serd::Node name = serd::make_string("(socket)");
	env.set_base_uri(*base_uri);
	reader.start_stream(f.get(), name, 1);

	// Make an AtomReader to call Ingen Interface methods based on Atom
	AtomReader ar(_world.uri_map(), _world.uris(), _world.log(), _iface);

	struct pollfd pfd{};
	pfd.fd      = _socket->fd();
	pfd.events  = POLLIN;
	pfd.revents = 0;

	while (!_exit_flag) {
		if (feof(f.get())) {
			break;  // Lost connection
		}

		// Wait for input to arrive at socket
		int ret = poll(&pfd, 1, -1);
		if (ret == -1 || (pfd.revents & (POLLERR|POLLHUP|POLLNVAL))) {
			on_hangup();
			break;  // Hangup
		} else if (!ret) {
			continue;  // No data, shouldn't happen
		}

		// Lock RDF world
		std::lock_guard<std::mutex> lock(_world.rdf_mutex());

		// Read until the next '.'
		auto st = reader.read_chunk();
		if (st == serd::Status::failure || !msg_node) {
			continue;  // Read no node (e.g. a directive)
		} else if (st != serd::Status::success) {
			_world.log().error("Read error: %1%\n", serd::strerror(st));
			continue;
		}

		// Build an LV2_Atom from the message
		auto atom = forge.read(*model, *msg_node);

		// Call _iface methods with forged atom
		ar.write(atom);

		// Reset everything for the next iteration
		msg_node.reset();
	}

	// Lock RDF world
	std::lock_guard<std::mutex> lock(_world.rdf_mutex());

	// Destroy everything
	f.reset();
	reader.finish();
	_socket.reset();
}

}  // namespace ingen
