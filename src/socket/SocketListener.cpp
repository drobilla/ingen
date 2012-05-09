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
#include <netinet/in.h>
#include <poll.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <string>
#include <sstream>

#include "ingen/Interface.hpp"
#include "ingen/shared/World.hpp"
#include "ingen/shared/AtomReader.hpp"
#include "sord/sordmm.hpp"
#include "sratom/sratom.h"

#include "../server/Engine.hpp"
#include "../server/EventWriter.hpp"
#include "SocketListener.hpp"
#include "SocketReader.hpp"

#define LOG(s) s << "[SocketListener] "

namespace Ingen {
namespace Socket {

bool
SocketListener::Socket::open(const std::string& uri,
                             int                domain,
                             struct sockaddr*   a,
                             socklen_t          s)
{
	addr     = a;
	addr_len = s;
	sock     = socket(domain, SOCK_STREAM, 0);
	if (sock == -1) {
		return false;
	}

	if (bind(sock, addr, addr_len) == -1) {
		LOG(Raul::error) << "Failed to bind " << uri << std::endl;
		return false;
	}

	if (listen(sock, 64) == -1) {
		LOG(Raul::error) << "Failed to listen on " << uri << std::endl;
		return false;
	} else {
		LOG(Raul::info) << "Listening on " << uri << std::endl;
	}

	return true;
}

int
SocketListener::Socket::accept()
{
	// Accept connection from client
	socklen_t        client_addr_len = addr_len;
	struct sockaddr* client_addr     = (struct sockaddr*)calloc(
		1, client_addr_len);

	int conn = ::accept(sock, client_addr, &client_addr_len);
	if (conn == -1) {
		LOG(Raul::error) << "Error accepting connection: "
		                 << strerror(errno) << std::endl;
	}

	return conn;
}

void
SocketListener::Socket::close()
{
	if (sock != -1) {
		::close(sock);
		sock = -1;
	}
}

SocketListener::SocketListener(Ingen::Shared::World& world)
	: _world(world)
{
	set_name("SocketListener");

	// Create UNIX socket
	_unix_path                = world.conf()->option("socket").get_string();
	std::string         unix_uri = "turtle+unix://" + _unix_path;
	struct sockaddr_un* uaddr    = (struct sockaddr_un*)calloc(
		1, sizeof(struct sockaddr_un));
	uaddr->sun_family = AF_UNIX;
	strncpy(uaddr->sun_path, _unix_path.c_str(), sizeof(uaddr->sun_path) - 1);
	if (!_unix_sock.open(unix_uri, AF_UNIX,
	                     (struct sockaddr*)uaddr,
	                     sizeof(struct sockaddr_un))) {
		LOG(Raul::error) << "Failed to create UNIX socket" << std::endl;
	}

	// Create TCP socket
	int port = world.conf()->option("engine-port").get_int();
	std::ostringstream ss;
	ss << "turtle+tcp:///localhost:";
	ss << port;
	struct sockaddr_in* naddr = (struct sockaddr_in*)calloc(
		1, sizeof(struct sockaddr_in));
	naddr->sin_family = AF_INET;
	naddr->sin_port   = htons(port);
	if (!_net_sock.open(ss.str(), AF_INET,
	                    (struct sockaddr*)naddr,
	                    sizeof(struct sockaddr_in))) {
		LOG(Raul::error) << "Failed to create TCP socket" << std::endl;
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
	Server::Engine* engine = (Server::Engine*)_world.local_engine().get();

	struct pollfd pfds[2];
	int           nfds = 0;
	if (_unix_sock.sock != -1) {
		pfds[nfds].fd      = _unix_sock.sock;
		pfds[nfds].events  = POLLIN;
		pfds[nfds].revents = 0;
		++nfds;
	}
	if (_net_sock.sock != -1) {
		pfds[nfds].fd      = _net_sock.sock;
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
			int conn = _unix_sock.accept();
			if (conn != -1) {
				// Make an new interface/thread to handle the connection
				new SocketReader(_world, *engine->interface(), conn);
			}
		}

		if (pfds[1].revents & POLLIN) {
			int conn = _net_sock.accept();
			if (conn != -1) {
				// Make an new interface/thread to handle the connection
				new SocketReader(_world, *engine->interface(), conn);
			}
		}
	}
}

}  // namespace Ingen
}  // namespace Socket
