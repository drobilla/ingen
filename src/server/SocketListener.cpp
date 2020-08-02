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

#include "SocketListener.hpp"

#include "Engine.hpp"
#include "SocketServer.hpp"

#include "ingen/Configuration.hpp"
#include "ingen/Log.hpp"
#include "ingen/World.hpp"
#include "raul/Socket.hpp"

#include <poll.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

namespace ingen {
namespace server {

static constexpr const char* const unix_scheme = "unix://";

static std::string
get_link_target(const char* link_path)
{
	// Stat the link to get the required size for the target path
	struct stat link_stat{};
	if (lstat(link_path, &link_stat)) {
		return std::string();
	}

	// Allocate buffer and read link target
	char* target = static_cast<char*>(calloc(1, link_stat.st_size + 1));
	if (readlink(link_path, target, link_stat.st_size) != -1) {
		std::string result(target);
		free(target);
		return result;
	}

	free(target);
	return std::string();
}

static void ingen_listen(Engine*       engine,
                         Raul::Socket* unix_sock,
                         Raul::Socket* net_sock);


SocketListener::SocketListener(Engine& engine)
	: unix_sock(Raul::Socket::Type::UNIX)
	, net_sock(Raul::Socket::Type::TCP)
	, thread(new std::thread(ingen_listen, &engine, &unix_sock, &net_sock))
{}

SocketListener::~SocketListener() {
	unix_sock.shutdown();
	net_sock.shutdown();
	thread->join();
	unlink(unix_sock.uri().substr(strlen(unix_scheme)).c_str());
}

static void
ingen_listen(Engine* engine, Raul::Socket* unix_sock, Raul::Socket* net_sock)
{
	ingen::World& world = engine->world();

	const std::string link_path(world.conf().option("socket").ptr<char>());
	const std::string unix_path(link_path + "." + std::to_string(getpid()));

	// Bind UNIX socket and create PID-less symbolic link
	const URI unix_uri(unix_scheme + unix_path);
	bool      make_link = true;
	if (!unix_sock->bind(unix_uri) || !unix_sock->listen()) {
		world.log().error("Failed to create UNIX socket\n");
		unix_sock->close();
		make_link = false;
	} else {
		const std::string old_path = get_link_target(link_path.c_str());
		if (!old_path.empty()) {
			const std::string suffix = old_path.substr(old_path.find_last_of('.') + 1);
			const pid_t       pid    = std::stoi(suffix);
			if (!kill(pid, 0)) {
				make_link = false;
				world.log().warn(
				        "Another Ingen instance is running at %1% => %2%\n",
				        link_path, old_path);
			} else {
				world.log().warn("Replacing old link %1% => %2%\n",
				                 link_path, old_path);
				unlink(link_path.c_str());
			}
		}

		if (make_link) {
			if (!symlink(unix_path.c_str(), link_path.c_str())) {
				world.log().info("Listening on %1%\n",
				                 (unix_scheme + link_path));
			} else {
				world.log().error("Failed to link %1% => %2% (%3%)\n",
				                  link_path, unix_path, strerror(errno));
			}
		} else {
			world.log().info("Listening on %1%\n", unix_uri);
		}
	}

	// Bind TCP socket
	const int port = world.conf().option("engine-port").get<int32_t>();
	std::ostringstream ss;
	ss << "tcp://*:" << port;
	if (!net_sock->bind(URI(ss.str())) || !net_sock->listen()) {
		world.log().error("Failed to create TCP socket\n");
		net_sock->close();
	} else {
		world.log().info("Listening on TCP port %1%\n", port);
	}

	if (unix_sock->fd() == -1 && net_sock->fd() == -1) {
		return;  // No sockets to listen to, exit thread
	}

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
			world.log().error("Poll error: %1%\n", strerror(errno));
			break;
		} else if (ret == 0) {
			world.log().warn("Poll returned with no data\n");
			continue;
		} else if ((pfds[0].revents & POLLHUP) || pfds[1].revents & POLLHUP) {
			break;
		}

		if (pfds[0].revents & POLLIN) {
			auto conn = unix_sock->accept();
			if (conn) {
				new SocketServer(world, *engine, conn);
			}
		}

		if (pfds[1].revents & POLLIN) {
			auto conn = net_sock->accept();
			if (conn) {
				new SocketServer(world, *engine, conn);
			}
		}
	}

	if (make_link) {
		unlink(link_path.c_str());
	}
}

} // namespace server
} // namespace ingen
