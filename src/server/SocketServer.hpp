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

#ifndef INGEN_SERVER_SOCKET_SERVER_HPP
#define INGEN_SERVER_SOCKET_SERVER_HPP

#include "raul/Socket.hpp"

#include "ingen/SocketReader.hpp"
#include "ingen/SocketWriter.hpp"

#include "EventWriter.hpp"

namespace Ingen {
namespace Server {

/** The server side of an Ingen socket connection. */
class SocketServer : public EventWriter, public SocketReader
{
public:
	SocketServer(World&             world,
	             Server::Engine&    engine,
	             SPtr<Raul::Socket> sock)
		: EventWriter(engine)
		, SocketReader(world, *this, sock)
		, _engine(engine)
		, _writer(new SocketWriter(world.uri_map(),
		                           world.uris(),
		                           sock->uri(),
		                           sock))
	{
		set_respondee(_writer);
		engine.register_client(_writer);
	}

	~SocketServer() {
		_engine.unregister_client(_writer);
	}

private:
	Server::Engine&    _engine;
	SPtr<SocketWriter> _writer;
};

}  // namespace Ingen
}  // namespace Socket

#endif  // INGEN_SERVER_SOCKET_SERVER_HPP
