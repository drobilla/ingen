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
#include <string>

#include "ingen/Interface.hpp"
#include "ingen/shared/AtomReader.hpp"
#include "ingen/shared/Configuration.hpp"
#include "ingen/shared/World.hpp"
#include "sord/sordmm.hpp"
#include "sratom/sratom.h"

#include "../server/Engine.hpp"
#include "../server/EventWriter.hpp"
#include "SocketListener.hpp"
#include "SocketServer.hpp"

#define LOG(s) s << "[SocketListener] "

namespace Ingen {
namespace Socket {

SocketListener::SocketListener(Ingen::Shared::World& world)
	: Raul::Thread("SocketListener")
	, _world(world)
	, _unix_sock(Socket::UNIX)
	, _net_sock(Socket::TCP)
{
	// Create UNIX socket
	_unix_path = world.conf().option("socket").get_string();
	const std::string unix_uri = "unix://" + _unix_path;
	if (!_unix_sock.bind(unix_uri) || !_unix_sock.listen()) {
		LOG(Raul::error) << "Failed to create UNIX socket" << std::endl;
		_unix_sock.close();
	}

	// Create TCP socket
	int port = world.conf().option("engine-port").get_int();
	std::ostringstream ss;
	ss << "tcp:///localhost:";
	ss << port;
	if (!_net_sock.bind(ss.str()) || !_net_sock.listen()) {
		LOG(Raul::error) << "Failed to create TCP socket" << std::endl;
		_net_sock.close();
	}

	start();
}

SocketListener::~SocketListener()
{
	stop();
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

	while (!_exit_flag) {
		// Wait for input to arrive at a socket
		int ret = poll(pfds, nfds, -1);
		if (ret == -1) {
			LOG(Raul::error) << "Poll error: " << strerror(errno) << std::endl;
			break;
		} else if (ret == 0) {
			LOG(Raul::error) << "Poll returned with no data" << std::endl;
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
