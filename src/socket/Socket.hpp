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

#ifndef INGEN_SOCKET_SOCKET_HPP
#define INGEN_SOCKET_SOCKET_HPP

#include <stdint.h>
#include <string.h>
#include <sys/socket.h>

#include <string>

#include "raul/SharedPtr.hpp"
#include "raul/Noncopyable.hpp"

namespace Ingen {
namespace Socket {

/** A safe and simple interface for UNIX or TCP sockets. */
class Socket : public Raul::Noncopyable {
public:
	enum Type {
		UNIX,
		TCP
	};

	static Type type_from_uri(const std::string& uri) {
		if (uri.substr(0, strlen("unix://")) == "unix://") {
			return UNIX;
		} else {
			return TCP;
		}
	}

	/** Create a new unbound/unconnected socket of a given type. */
	explicit Socket(Type t);

	/** Wrap an existing open socket. */
	Socket(Type               t,
	       const std::string& uri,
	       struct sockaddr*   addr,
	       socklen_t          addr_len,
	       int                fd);

	~Socket() { close(); }

	/** Bind a server socket to an address.
	 * @param uri Address URI, e.g. unix:///tmp/foo or tcp://somehost:1234
	 * @return True on success.
	 */
	bool bind(const std::string& uri);

	/** Connect a client socket to a server address.
	 * @param uri Address URI, e.g. unix:///tmp/foo or tcp://somehost:1234
	 * @return True on success.
	 */
	bool connect(const std::string& uri);

	/** Mark server socket as passive to listen for incoming connections.
	 * @return True on success.
	 */
	bool listen();

	/** Accept a connection.
	 * @return An new open socket for the connection.
	 */
	SharedPtr<Socket> accept();

	/** Return the file descriptor for the socket. */
	int fd() { return _sock; }

	const std::string& uri() const { return _uri; }

	/** Close the socket. */
	void close();

private:
	bool set_addr(const std::string& uri);

	Type             _type;
	std::string      _uri;
	struct sockaddr* _addr;
	socklen_t        _addr_len;
	int              _sock;
};

}  // namespace Socket
}  // namespace Ingen

#endif  // INGEN_SOCKET_SOCKET_HPP
