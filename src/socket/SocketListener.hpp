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

#include "Socket.hpp"

namespace Ingen {

class Interface;

namespace Shared { class World; }

namespace Socket {

class SocketListener : public Raul::Thread
{
public:
	explicit SocketListener(Ingen::Shared::World& world);
	~SocketListener();

private:
	virtual void _run();

	Ingen::Shared::World& _world;
	std::string           _unix_path;
	Socket                _unix_sock;
	Socket                _net_sock;
};

}  // namespace Ingen
}  // namespace Socket
