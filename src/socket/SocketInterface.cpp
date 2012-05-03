/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

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
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "ingen/Interface.hpp"
#include "ingen/shared/World.hpp"
#include "ingen/shared/AtomReader.hpp"
#include "sord/sordmm.hpp"
#include "sratom/sratom.h"
#include "SocketInterface.hpp"

#include "../server/Event.hpp"
#include "../server/PostProcessor.hpp"
#include "../server/ThreadManager.hpp"

#define LOG(s) s << "[SocketInterface] "

namespace Ingen {
namespace Socket {

SocketInterface::SocketInterface(Ingen::Shared::World& world, int conn)
	: _world(world)
	, _iface(*(Server::Engine*)world.local_engine().get(), *this)
	, _event(NULL)
	, _conn(conn)
{
	// Set connection to non-blocking so parser can read until EOF
	// and not block indefinitely waiting for more input
	fcntl(_conn, F_SETFL, fcntl(_conn, F_GETFL, 0) | O_NONBLOCK);

	set_name("SocketInterface");
	start();
}

SocketInterface::~SocketInterface()
{
	stop();
	join();
	close(_conn);
}

void
SocketInterface::event(Server::Event* ev)
{
	assert(!_event);
	ev->pre_process();
	_event = ev;
}

bool
SocketInterface::process(Server::PostProcessor&  dest,
                         Server::ProcessContext& context,
                         bool                    limit)
{
	if (_event) {
		_event->execute(context);
		dest.append(_event, _event);
		_event = NULL;
	}
	if (_conn == -1) {
		return false;
	}
	return true;
}

void
SocketInterface::_run()
{
	Thread::set_context(Server::THREAD_PRE_PROCESS);
	while (!_exit_flag) {
		// Set up a reader to parse the Turtle message into a model
		Sord::World* world  = _world.rdf_world();
		SerdEnv*     env    = world->prefixes().c_obj();
		SordModel*   model  = sord_new(world->c_obj(), SORD_SPO, false);
		SerdReader*  reader = sord_new_reader(model, env, SERD_TURTLE, NULL);
		// Set base URI to path: so e.g. </foo/bar> will be a path
		SordNode* base_uri = sord_new_uri(
			world->c_obj(), (const uint8_t*)"path:");
		serd_env_set_base_uri(env, sord_node_to_serd_node(base_uri));

		LV2_URID_Map* map = &_world.lv2_uri_map()->urid_map_feature()->urid_map;

		// Set up sratom to build an LV2_Atom from the model
		Sratom*        sratom = sratom_new(map);
		SerdChunk      chunk  = { NULL, 0 };
		LV2_Atom_Forge forge;
		lv2_atom_forge_init(&forge, map);
		lv2_atom_forge_set_sink(
			&forge, sratom_forge_sink, sratom_forge_deref, &chunk);

		// Read directly from the connection with serd
		FILE* f = fdopen(_conn, "r");
		if (!f) {
			LOG(Raul::error) << "Failed to open connection " << _conn
			                 << "(" << strerror(errno) << ")" << std::endl;
			// Connection gone, exit
			break;
		}

		serd_reader_read_file_handle(reader, f, (const uint8_t*)"(socket)");

		// FIXME: Sratom needs work to be able to read resources
		SordNode* msg_node = sord_new_blank(
			world->c_obj(), (const uint8_t*)"genid1");

		// Build an LV2_Atom at chunk.buf from the message
		sratom_read(sratom, &forge, world->c_obj(), model, msg_node);

		// Make an AtomReader to read that atom and do Ingen things
		Shared::AtomReader ar(*_world.lv2_uri_map().get(),
		                      *_world.uris().get(),
		                      _world.forge(),
		                      _iface);

		// Call _iface methods based on atom content
		ar.write((LV2_Atom*)chunk.buf);

		// Respond and close connection
		write(_conn, "OK", 2);
		fclose(f);

		sratom_free(sratom);
		sord_node_free(world->c_obj(), msg_node);
		serd_reader_free(reader);
		sord_free(model);
	}

	_conn = -1;
}

}  // namespace Ingen
}  // namespace Socket
