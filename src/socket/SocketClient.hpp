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

#ifndef INGEN_SOCKET_SOCKET_CLIENT_HPP
#define INGEN_SOCKET_SOCKET_CLIENT_HPP

#include "SocketReader.hpp"
#include "SocketWriter.hpp"

namespace Ingen {
namespace Socket {

/** The client side of an Ingen socket connection. */
class SocketClient : public SocketWriter
{
public:
	SocketClient(Shared::World&       world,
	             const std::string&   uri,
	             SharedPtr<Socket>    sock,
	             SharedPtr<Interface> respondee)
		: SocketWriter(world.uri_map(), world.uris(), uri, sock)
		, _respondee(respondee)
		, _reader(world, *respondee.get(), sock)
	{
		_reader.start();
	}

	virtual SharedPtr<Interface> respondee() const {
		return _respondee;
	}

	virtual void set_respondee(SharedPtr<Interface> respondee) {
		_respondee = respondee;
	}

private:
	SharedPtr<Interface> _respondee;
	SocketReader         _reader;
};

}  // namespace Socket
}  // namespace Ingen

#endif  // INGEN_SOCKET_SOCKET_CLIENT_HPP
