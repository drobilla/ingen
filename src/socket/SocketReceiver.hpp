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

#include <string>

#include "raul/SharedPtr.hpp"
#include "raul/Thread.hpp"

namespace Ingen {

class Interface;

namespace Shared { class World; }

namespace Socket {

class SocketReceiver : public Raul::Thread
{
public:
	SocketReceiver(Ingen::Shared::World& world,
	               SharedPtr<Interface>  iface);

	~SocketReceiver();

private:
	virtual void _run();

	Ingen::Shared::World& _world;
	SharedPtr<Interface>  _iface;
	std::string           _sock_path;
	int                   _sock;
};

}  // namespace Ingen
}  // namespace Socket
