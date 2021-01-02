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

#include "raul/Socket.hpp"

#include <memory>
#include <thread>

namespace ingen {
namespace server {

class Engine;

/** Listens on main sockets and spawns socket servers for new connections. */
class SocketListener
{
public:
	SocketListener(Engine& engine);
	~SocketListener();

private:
	raul::Socket                 unix_sock;
	raul::Socket                 net_sock;
	std::unique_ptr<std::thread> thread;
};

} // namespace server
} // namespace ingen
