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
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>

#include <string>
#include <sstream>

#include "raul/log.hpp"

#include "Socket.hpp"

#define LOG(s) s << "[Socket] "

namespace Ingen {
namespace Socket {

bool
Socket::open_unix(const std::string& uri, const std::string& path)
{
	if ((_sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		return false;
	}

	struct sockaddr_un* uaddr = (struct sockaddr_un*)calloc(
		1, sizeof(struct sockaddr_un));
	uaddr->sun_family = AF_UNIX;
	strncpy(uaddr->sun_path, path.c_str(), sizeof(uaddr->sun_path) - 1);
	_uri      = uri;
	_addr     = (sockaddr*)uaddr;
	_addr_len = sizeof(struct sockaddr_un);

	return bind();
}

bool
Socket::open_tcp(const std::string& uri, uint16_t port)
{
	if ((_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		return false;
	}

	struct sockaddr_in* naddr = (struct sockaddr_in*)calloc(
		1, sizeof(struct sockaddr_in));
	naddr->sin_family = AF_INET;
	naddr->sin_port   = htons(port);
	_uri      = uri;
	_addr     = (sockaddr*)naddr;
	_addr_len = sizeof(struct sockaddr_in);

	return bind();
}

bool
Socket::bind()
{
	if (::bind(_sock, _addr, _addr_len) == -1) {
		LOG(Raul::error) << "Failed to bind " << _uri
		                 << ": " << strerror(errno) << std::endl;
		return false;
	}
	return true;
}

bool
Socket::listen()
{
	if (::listen(_sock, 64) == -1) {
		LOG(Raul::error) << "Failed to listen on " << _uri << std::endl;
		return false;
	} else {
		LOG(Raul::info) << "Listening on " << _uri << std::endl;
		return true;
	}
}

int
Socket::accept()
{
	// Accept connection from client
	socklen_t        client_addr_len = _addr_len;
	struct sockaddr* client_addr     = (struct sockaddr*)calloc(
		1, client_addr_len);

	int conn = ::accept(_sock, client_addr, &client_addr_len);
	if (conn == -1) {
		LOG(Raul::error) << "Error accepting connection: "
		                 << strerror(errno) << std::endl;
	}

	return conn;
}

void
Socket::close()
{
	if (_sock != -1) {
		::close(_sock);
		_sock = -1;
	}
}

}  // namespace Ingen
}  // namespace Socket
