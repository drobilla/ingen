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
#include <thread>

#include "ingen/Configuration.hpp"
#include "ingen/Module.hpp"
#include "ingen/World.hpp"
#include "raul/Socket.hpp"

#include "../server/Engine.hpp"
#include "../server/EventWriter.hpp"

#include "SocketServer.hpp"

#define UNIX_SCHEME "unix://"

namespace Ingen {
namespace Socket {

static void
ingen_listen(Ingen::World* world,
             Raul::Socket* unix_sock,
             Raul::Socket* net_sock)
{
	const std::string unix_path(world->conf().option("socket").ptr<char>());

	SPtr<Server::Engine> engine = dynamic_ptr_cast<Server::Engine>(
		world->engine());

	// Bind UNIX socket
	const Raul::URI unix_uri(UNIX_SCHEME + unix_path);
	if (!unix_sock->bind(unix_uri) || !unix_sock->listen()) {
		world->log().error("Failed to create UNIX socket\n");
		unix_sock->close();
	}
	world->log().info(fmt("Listening on socket %1%\n") % unix_uri);

	// Bind TCP socket
	const int port = world->conf().option("engine-port").get<int32_t>();
	std::ostringstream ss;
	ss << "tcp://localhost:";
	ss << port;
	if (!net_sock->bind(Raul::URI(ss.str())) || !net_sock->listen()) {
		world->log().error("Failed to create TCP socket\n");
		net_sock->close();
	}
	world->log().info(fmt("Listening on TCP port %1%\n") % port);

	struct pollfd pfds[2];
	int           nfds = 0;
	if (unix_sock->fd() != -1) {
		pfds[nfds].fd      = unix_sock->fd();
		pfds[nfds].events  = POLLIN;
		pfds[nfds].revents = 0;
		++nfds;
	}
	if (net_sock->fd() != -1) {
		pfds[nfds].fd      = net_sock->fd();
		pfds[nfds].events  = POLLIN;
		pfds[nfds].revents = 0;
		++nfds;
	}

	while (true) {
		// Wait for input to arrive at a socket
		const int ret = poll(pfds, nfds, -1);
		if (ret == -1) {
			world->log().error(fmt("Poll error: %1%\n") % strerror(errno));
			break;
		} else if ((pfds[0].revents & POLLHUP) || pfds[1].revents & POLLHUP) {
			break;
		} else if (ret == 0) {
			world->log().error("Poll returned with no data\n");
			continue;
		}

		if (pfds[0].revents & POLLIN) {
			SPtr<Raul::Socket> conn = unix_sock->accept();
			if (conn) {
				new SocketServer(*world, *engine, conn);
			}
		}

		if (pfds[1].revents & POLLIN) {
			SPtr<Raul::Socket> conn = net_sock->accept();
			if (conn) {
				new SocketServer(*world, *engine, conn);
			}
		}
	}
}

struct ServerModule : public Ingen::Module
{
	ServerModule()
		: unix_sock(Raul::Socket::Type::UNIX)
		, net_sock(Raul::Socket::Type::TCP)
	{}

	~ServerModule() {
		unix_sock.shutdown();
		net_sock.shutdown();
		thread->join();
		unlink(unix_sock.uri().substr(strlen(UNIX_SCHEME)).c_str());
	}

	void load(World* world) {
		world = world;
		thread = std::unique_ptr<std::thread>(
			new std::thread(ingen_listen, world, &unix_sock, &net_sock));
	}

	World*                       world;
	Raul::Socket                 unix_sock;
	Raul::Socket                 net_sock;
	std::unique_ptr<std::thread> thread;
};

} // namespace Socket
} // namespace Ingen

extern "C" {

Ingen::Module*
ingen_module_load()
{
	return new Ingen::Socket::ServerModule();
}

} // extern "C"
