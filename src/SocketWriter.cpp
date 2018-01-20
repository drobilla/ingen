/*
  This file is part of Ingen.
  Copyright 2012-2016 David Robillard <http://drobilla.net/>

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
#include <sys/types.h>
#include <sys/socket.h>

#include "ingen/SocketWriter.hpp"
#include "raul/Socket.hpp"

#ifndef MSG_NOSIGNAL
#    define MSG_NOSIGNAL 0
#endif

namespace Ingen {

SocketWriter::SocketWriter(URIMap&            map,
                           URIs&              uris,
                           const URI&         uri,
                           SPtr<Raul::Socket> sock)
	: TurtleWriter(map, uris, uri)
	, _socket(std::move(sock))
{}

size_t
SocketWriter::text_sink(const void* buf, size_t len)
{
	ssize_t ret = send(_socket->fd(), buf, len, MSG_NOSIGNAL);
	if (ret < 0) {
		return 0;
	}
	return ret;
}

void
SocketWriter::bundle_end()
{
	TurtleWriter::bundle_end();

	// Send a NULL byte to indicate end of bundle
	const char end[] = { 0 };
	send(_socket->fd(), end, 1, MSG_NOSIGNAL);
}

} // namespace Ingen
