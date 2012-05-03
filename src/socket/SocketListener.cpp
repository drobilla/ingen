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

#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "ingen/Interface.hpp"
#include "ingen/shared/World.hpp"
#include "ingen/shared/AtomReader.hpp"
#include "sord/sordmm.hpp"
#include "sratom/sratom.h"
#include "SocketListener.hpp"

#define LOG(s) s << "[SocketListener] "

namespace Ingen {
namespace Socket {

SocketListener::SocketListener(Ingen::Shared::World& world,
                               SharedPtr<Interface>  iface)
	: _world(world)
	, _iface(iface)
{
	// Create server socket
	_sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (_sock == -1) {
		LOG(Raul::error) << "Failed to create socket" << std::endl;
		return;
	}

	_sock_path = world.conf()->option("socket").get_string();

	// Make server socket address
	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, _sock_path.c_str(), sizeof(addr.sun_path) - 1);

	// Bind socket to address
	if (bind(_sock, (struct sockaddr*)&addr,
	         sizeof(struct sockaddr_un)) == -1) {
		LOG(Raul::error) << "Failed to bind socket" << std::endl;
		return;
	}

	// Mark socket as a passive socket for accepting incoming connections
	if (listen(_sock, 64) == -1) {
		LOG(Raul::error) << "Failed to listen on socket" << std::endl;
	}

	LOG(Raul::info) << "Listening on socket at " << _sock_path << std::endl;
	start();
}

SocketListener::~SocketListener()
{
	stop();
	join();
	close(_sock);
	unlink(_sock_path.c_str());
}

void
SocketListener::_run()
{
	while (!_exit_flag) {
		// Accept connection from client
		socklen_t          client_addr_size = sizeof(struct sockaddr_un);
		struct sockaddr_un client_addr;
		int conn = accept(_sock, (struct sockaddr*)&client_addr,
		                  &client_addr_size);
		if (conn == -1) {
			LOG(Raul::error) << "Error accepting connection" << std::endl;
			continue;
		}

		// Set connection to non-blocking so parser can read until EOF
		// and not block indefinitely waiting for more input
		fcntl(conn, F_SETFL, fcntl(conn, F_GETFL, 0) | O_NONBLOCK);

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
		FILE* f = fdopen(conn, "r");
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
		                      *_iface.get());

		// Call _iface methods based on atom content
		ar.write((LV2_Atom*)chunk.buf);

		// Respond and close connection
		write(conn, "OK", 2);
		fclose(f);
		close(conn);

		sratom_free(sratom);
		sord_node_free(world->c_obj(), msg_node);
		serd_reader_free(reader);
		sord_free(model);
	}
}

}  // namespace Ingen
}  // namespace Socket
