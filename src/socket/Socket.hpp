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
#include <sys/socket.h>

#include "raul/Noncopyable.hpp"
#include "raul/URI.hpp"

#include "ingen/types.hpp"

namespace Ingen {
namespace Socket {

/** A safe and simple interface for UNIX or TCP sockets. */
class Socket : public Raul::Noncopyable {
public:
	enum class Type {
		UNIX,
		TCP
	};

	static Type type_from_uri(const Raul::URI uri) {
		return (uri.scheme() == "unix") ? Type::UNIX : Type::TCP;
	}

	/** Create a new unbound/unconnected socket of a given type. */
	explicit Socket(Type t);

	/** Wrap an existing open socket. */
	Socket(Type             t,
	       const Raul::URI& uri,
	       struct sockaddr* addr,
	       socklen_t        addr_len,
	       int              fd);

	~Socket();

	/** Bind a server socket to an address.
	 * @param uri Address URI, e.g. unix:///tmp/foo or tcp://somehost:1234
	 * @return True on success.
	 */
	bool bind(const Raul::URI& uri);

	/** Connect a client socket to a server address.
	 * @param uri Address URI, e.g. unix:///tmp/foo or tcp://somehost:1234
	 * @return True on success.
	 */
	bool connect(const Raul::URI& uri);

	/** Mark server socket as passive to listen for incoming connections.
	 * @return True on success.
	 */
	bool listen();

	/** Accept a connection.
	 * @return An new open socket for the connection.
	 */
	SPtr<Socket> accept();

	/** Return the file descriptor for the socket. */
	int fd() { return _sock; }

	const Raul::URI& uri() const { return _uri; }

	/** Close the socket. */
	void close();

	/** Shut down the socket.
	 * This terminates any connections associated with the sockets, and will
	 * (unlike close()) cause a poll on the socket to return.
	 */
	void shutdown();

private:
	bool set_addr(const Raul::URI& uri);

	Type             _type;
	Raul::URI        _uri;
	struct sockaddr* _addr;
	socklen_t        _addr_len;
	int              _sock;
};

}  // namespace Socket
}  // namespace Ingen

#endif  // INGEN_SOCKET_SOCKET_HPP
