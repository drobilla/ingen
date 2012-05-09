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

#include <stdint.h>
#include <sys/socket.h>

#include <string>

namespace Ingen {
namespace Socket {

class Socket {
public:
	Socket() : _addr(NULL), _addr_len(0), _sock(-1) {}
	~Socket() { close(); }

	/** Open UNIX socket and bind to address.
	 * @param uri URI used for identification and log output.
	 * @param path Socket path.
	 * @return True on success
	 */
	bool open_unix(const std::string& uri, const std::string& path);

	/** Open TCP socket and bind to address.
	 * @param uri URI used for identification and log output.
	 * @param port Port number.
	 * @return True on success
	 */
	bool open_tcp(const std::string& uri, uint16_t port);

	/** Mark server socket as passive to listen for incoming connections.
	 * @return True on success.
	 */
	bool listen();

	/** Accept a connection.
	 * @return The socket file descriptor, or -1 on error.
	 */
	int accept();

	/** Return the file descriptor for the socket. */
	int fd() { return _sock; }

	/** Close the socket. */
	void close();

private:
	bool bind();

	std::string      _uri;
	struct sockaddr* _addr;
	socklen_t        _addr_len;
	int              _sock;
};

}  // namespace Socket
}  // namespace Ingen
