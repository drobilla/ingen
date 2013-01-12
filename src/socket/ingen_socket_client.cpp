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

#include "ingen/Log.hpp"
#include "ingen/Module.hpp"
#include "ingen/World.hpp"

#include "Socket.hpp"
#include "SocketClient.hpp"

static Ingen::SPtr<Ingen::Interface>
new_socket_interface(Ingen::World*                 world,
                     const Raul::URI&              uri,
                     Ingen::SPtr<Ingen::Interface> respondee)
{
	Ingen::SPtr<Ingen::Socket::Socket> sock(
		new Ingen::Socket::Socket(Ingen::Socket::Socket::type_from_uri(uri)));
	if (!sock->connect(uri)) {
		world->log().error(Raul::fmt("Failed to connect <%1%> (%2%)\n")
		                   % sock->uri() % strerror(errno));
		return Ingen::SPtr<Ingen::Interface>();
	}
	Ingen::Socket::SocketClient* client = new Ingen::Socket::SocketClient(
		*world, uri, sock, respondee);
	return Ingen::SPtr<Ingen::Interface>(client);
}

struct IngenSocketClientModule : public Ingen::Module {
	void load(Ingen::World* world) {
		world->add_interface_factory("unix", &new_socket_interface);
		world->add_interface_factory("tcp", &new_socket_interface);
	}
};

extern "C" {

Ingen::Module*
ingen_module_load()
{
	return new IngenSocketClientModule();
}

} // extern "C"
