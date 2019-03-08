/*
  This file is part of Ingen.
  Copyright 2012-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_CLIENT_SOCKET_CLIENT_HPP
#define INGEN_CLIENT_SOCKET_CLIENT_HPP

#include "ingen/SocketReader.hpp"
#include "ingen/SocketWriter.hpp"
#include "ingen/ingen.h"
#include "raul/Socket.hpp"

namespace ingen {
namespace client {

/** The client side of an Ingen socket connection. */
class INGEN_API SocketClient : public SocketWriter
{
public:
	SocketClient(World&             world,
	             const URI&         uri,
	             SPtr<Raul::Socket> sock,
	             SPtr<Interface>    respondee)
		: SocketWriter(world.uri_map(), world.uris(), uri, sock)
		, _respondee(respondee)
		, _reader(world, *respondee.get(), sock)
	{}

	SPtr<Interface> respondee() const override {
		return _respondee;
	}

	void set_respondee(SPtr<Interface> respondee) override {
		_respondee = respondee;
	}

	static SPtr<ingen::Interface>
	new_socket_interface(ingen::World&          world,
	                     const URI&             uri,
	                     SPtr<ingen::Interface> respondee)
	{
		const Raul::Socket::Type type = (uri.scheme() == "unix"
		                                 ? Raul::Socket::Type::UNIX
		                                 : Raul::Socket::Type::TCP);

		SPtr<Raul::Socket> sock(new Raul::Socket(type));
		if (!sock->connect(uri)) {
			world.log().error(fmt("Failed to connect <%1%> (%2%)\n")
			                   % sock->uri() % strerror(errno));
			return SPtr<Interface>();
		}
		return SPtr<Interface>(new SocketClient(world, uri, sock, respondee));
	}

	static void register_factories(World& world) {
		world.add_interface_factory("unix", &new_socket_interface);
		world.add_interface_factory("tcp", &new_socket_interface);
	}

private:
	SPtr<Interface> _respondee;
	SocketReader    _reader;
};

}  // namespace client
}  // namespace ingen

#endif  // INGEN_CLIENT_SOCKET_CLIENT_HPP
