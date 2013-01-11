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
#include <poll.h>

#include <sstream>

#include "ingen/AtomReader.hpp"
#include "ingen/Configuration.hpp"
#include "ingen/Interface.hpp"
#include "ingen/Log.hpp"
#include "ingen/World.hpp"
#include "sord/sordmm.hpp"
#include "sratom/sratom.h"

#include "../server/Engine.hpp"
#include "../server/EventWriter.hpp"
#include "SocketListener.hpp"
#include "SocketServer.hpp"

namespace Ingen {
namespace Socket {

SocketListener::SocketListener(Ingen::World& world)
	: Raul::Thread()
	, _world(world)
	, _unix_sock(Socket::Type::UNIX)
	, _net_sock(Socket::Type::TCP)
{
	// Create UNIX socket
	_unix_path = world.conf().option("socket").get_string();
	const Raul::URI unix_uri("unix://" + _unix_path);
	if (!_unix_sock.bind(unix_uri) || !_unix_sock.listen()) {
		_world.log().error("Failed to create UNIX socket\n");
		_unix_sock.close();
	}
	_world.log().info(Raul::fmt("Listening on socket %1%\n") % unix_uri);

	// Create TCP socket
	int port = world.conf().option("engine-port").get_int32();
	std::ostringstream ss;
	ss << "tcp://localhost:";
	ss << port;
	if (!_net_sock.bind(Raul::URI(ss.str())) || !_net_sock.listen()) {
		_world.log().error("Failed to create TCP socket\n");
		_net_sock.close();
	}
	_world.log().info(Raul::fmt("Listening on TCP port %1%\n") % port);

	start();
}

SocketListener::~SocketListener()
{
	_exit_flag = true;
	_unix_sock.shutdown();
	_net_sock.shutdown();
	join();
	_unix_sock.close();
	_net_sock.close();
	unlink(_unix_path.c_str());
}

void
SocketListener::_run()
{
	Server::Engine* engine = (Server::Engine*)_world.engine().get();

	struct pollfd pfds[2];
	int           nfds = 0;
	if (_unix_sock.fd() != -1) {
		pfds[nfds].fd      = _unix_sock.fd();
		pfds[nfds].events  = POLLIN;
		pfds[nfds].revents = 0;
		++nfds;
	}
	if (_net_sock.fd() != -1) {
		pfds[nfds].fd      = _net_sock.fd();
		pfds[nfds].events  = POLLIN;
		pfds[nfds].revents = 0;
		++nfds;
	}

	while (true) {
		// Wait for input to arrive at a socket
		int ret = poll(pfds, nfds, -1);
		if (_exit_flag) {
			break;
		} else if (ret == -1) {
			_world.log().error(Raul::fmt("Poll error: %1%\n") % strerror(errno));
			break;
		} else if (ret == 0) {
			_world.log().error("Poll returned with no data\n");
			continue;
		}

		if (pfds[0].revents & POLLIN) {
			SharedPtr<Socket> conn = _unix_sock.accept();
			if (conn) {
				new SocketServer(_world, *engine, conn);
			}
		}

		if (pfds[1].revents & POLLIN) {
			SharedPtr<Socket> conn = _net_sock.accept();
			if (conn) {
				new SocketServer(_world, *engine, conn);
			}
		}
	}
}

}  // namespace Ingen
}  // namespace Socket
