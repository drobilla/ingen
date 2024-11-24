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

#include "Broadcaster.hpp"

#include "BlockFactory.hpp"
#include "PluginImpl.hpp"

#include <ingen/Interface.hpp>

#include <cstddef>
#include <memory>
#include <utility>

namespace ingen::server {

Broadcaster::~Broadcaster()
{
	const std::lock_guard<std::mutex> lock{_clients_mutex};
	_clients.clear();
	_broadcastees.clear();
}

/** Register a client to receive messages over the notification band.
 */
void
Broadcaster::register_client(const std::shared_ptr<Interface>& client)
{
	const std::lock_guard<std::mutex> lock{_clients_mutex};
	_clients.insert(client);
}

/** Remove a client from the list of registered clients.
 *
 * @return true if client was found and removed.
 */
bool
Broadcaster::unregister_client(const std::shared_ptr<Interface>& client)
{
	const std::lock_guard<std::mutex> lock{_clients_mutex};
	const size_t erased = _clients.erase(client);
	_broadcastees.erase(client);
	return (erased > 0);
}

void
Broadcaster::set_broadcast(const std::shared_ptr<Interface>& client,
                           bool                              broadcast)
{
	if (broadcast) {
		_broadcastees.insert(client);
	} else {
		_broadcastees.erase(client);
	}
	_must_broadcast.store(!_broadcastees.empty());
}

void
Broadcaster::send_plugins(const BlockFactory::Plugins& plugins)
{
	const std::lock_guard<std::mutex> lock{_clients_mutex};
	for (const auto& c : _clients) {
		send_plugins_to(c.get(), plugins);
	}
}

void
Broadcaster::send_plugins_to(Interface*                   client,
                             const BlockFactory::Plugins& plugins)
{
	client->bundle_begin();

	for (const auto& p : plugins) {
		const auto& plugin = p.second;
		client->put(plugin->uri(), plugin->properties());
	}

	client->bundle_end();
}

} // namespace ingen::server
