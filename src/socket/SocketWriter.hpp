/*
  This file is part of Ingen.
  Copyright 2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_SOCKET_SOCKET_WRITER_HPP
#define INGEN_SOCKET_SOCKET_WRITER_HPP

#include <stdint.h>

#include "ingen/Interface.hpp"
#include "ingen/shared/AtomSink.hpp"
#include "ingen/shared/AtomWriter.hpp"
#include "raul/URI.hpp"
#include "raul/SharedPtr.hpp"
#include "sratom/sratom.h"

#include "Socket.hpp"

namespace Ingen {
namespace Socket {

/** An Interface that writes Turtle messages to a socket.
 */
class SocketWriter : public Shared::AtomWriter, public Shared::AtomSink
{
public:
	SocketWriter(Shared::LV2URIMap& map,
	             Shared::URIs&      uris,
	             const Raul::URI&   uri,
	             SharedPtr<Socket>  sock);

	~SocketWriter();

	void write(const LV2_Atom* msg);

	int       fd()        { return _socket->fd(); }
	Raul::URI uri() const { return _uri; }

protected:
	Shared::LV2URIMap& _map;
	Sratom*            _sratom;
	SerdNode           _base;
	SerdURI            _base_uri;
	SerdEnv*           _env;
	SerdWriter*        _writer;
	Raul::URI          _uri;
	SharedPtr<Socket>  _socket;
};

}  // namespace Socket
}  // namespace Ingen

#endif  // INGEN_SOCKET_SOCKET_WRITER_HPP
