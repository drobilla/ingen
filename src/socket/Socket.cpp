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
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <string>
#include <sstream>

#include "raul/log.hpp"

#include "Socket.hpp"

#define LOG(s) (s("[Socket] "))

namespace Ingen {
namespace Socket {

#ifndef NI_MAXHOST
#    define NI_MAXHOST 1025
#endif
#ifndef NI_MAXSERV
#    define NI_MAXSERV 32
#endif

Socket::Socket(Type t)
	: _type(t)
	, _addr(NULL)
	, _addr_len(0)
	, _sock(-1)
{
	switch (t) {
	case UNIX:
		_sock = socket(AF_UNIX, SOCK_STREAM, 0);
		break;
	case TCP:
		_sock = socket(AF_INET, SOCK_STREAM, 0);
		break;
	}
}

Socket::Socket(Type               t,
               const std::string& uri,
               struct sockaddr*   addr,
               socklen_t          addr_len,
               int                fd)
	: _type(t)
	, _uri(uri)
	, _addr(addr)
	, _addr_len(addr_len)
	, _sock(fd)
{
}

bool
Socket::set_addr(const std::string& uri)
{
	if (_type == UNIX && uri.substr(0, strlen("unix://")) == "unix://") {
		const std::string   path  = uri.substr(strlen("unix://"));
		struct sockaddr_un* uaddr = (struct sockaddr_un*)calloc(
			1, sizeof(struct sockaddr_un));
		uaddr->sun_family = AF_UNIX;
		strncpy(uaddr->sun_path, path.c_str(), sizeof(uaddr->sun_path) - 1);
		_uri      = uri;
		_addr     = (sockaddr*)uaddr;
		_addr_len = sizeof(struct sockaddr_un);
		return true;
	} else if (_type == TCP && uri.find("://") != std::string::npos) {
		const std::string no_scheme = uri.substr(uri.find("://") + 4);
		const size_t      port_sep  = no_scheme.find(':');
		if (port_sep == std::string::npos) {
			return false;
		}

		const std::string host = no_scheme.substr(0, port_sep);
		const std::string port = no_scheme.substr(port_sep + 1).c_str();

		struct addrinfo* ainfo;
		int              st = 0;
		if ((st = getaddrinfo(host.c_str(), port.c_str(), NULL, &ainfo))) {
			LOG(Raul::error)(Raul::fmt("Error in getaddrinfo: %1%\n")
			                           % gai_strerror(st));
			return false;
		}

		_uri      = uri;
		_addr     = (struct sockaddr*)malloc(ainfo->ai_addrlen);
		_addr_len = ainfo->ai_addrlen;
		memcpy(_addr, ainfo->ai_addr, ainfo->ai_addrlen);
		return true;
	}
	return false;
}

bool
Socket::bind(const std::string& uri)
{
	if (set_addr(uri) && ::bind(_sock, _addr, _addr_len) != -1) {
		return true;
	}
	
	LOG(Raul::error)(Raul::fmt("Failed to bind <%1%> (%2%)\n")
	                 % _uri % strerror(errno));
	return false;
}

bool
Socket::connect(const std::string& uri)
{
	if (set_addr(uri) && ::connect(_sock, _addr, _addr_len) != -1) {
		return true;
	}
	
	LOG(Raul::error)(Raul::fmt("Failed to connect <%1%> (%2%)\n")
	                 % _uri % strerror(errno));
	return false;
}

bool
Socket::listen()
{
	if (::listen(_sock, 64) == -1) {
		LOG(Raul::error)(Raul::fmt("Failed to listen on %1%\n") % _uri);
		return false;
	} else {
		LOG(Raul::info)(Raul::fmt("Listening on %1%\n") % _uri);
		return true;
	}
}

SharedPtr<Socket>
Socket::accept()
{
	socklen_t        client_addr_len = _addr_len;
	struct sockaddr* client_addr     = (struct sockaddr*)calloc(
		1, client_addr_len);

	int conn = ::accept(_sock, client_addr, &client_addr_len);
	if (conn == -1) {
		LOG(Raul::error)(Raul::fmt("Error accepting connection: %1%\n")
		                 % strerror(errno));
		return SharedPtr<Socket>();
	}

	std::string client_uri = _uri;
	char        host[NI_MAXHOST];
	if (getnameinfo(client_addr, client_addr_len,
	                host, sizeof(host), NULL, 0, 0)) {
		client_uri = _uri.substr(0, _uri.find(":") + 1) + host;
	}
	
	return SharedPtr<Socket>(
		new Socket(_type, client_uri, client_addr, client_addr_len, conn));
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
