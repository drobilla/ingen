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

#include "../server/Engine.hpp"
#include "../server/EventWriter.hpp"
#include "SocketListener.hpp"
#include "SocketInterface.hpp"

#define LOG(s) s << "[SocketListener] "

namespace Ingen {
namespace Socket {

SocketListener::SocketListener(Ingen::Shared::World& world)
	: _world(world)
{
	set_name("SocketListener");

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

		// Make an new interface/thread to handle the connection
		Server::Engine* engine = (Server::Engine*)_world.local_engine().get();
		new SocketInterface(_world, *engine->interface(), conn);
	}
}

}  // namespace Ingen
}  // namespace Socket
