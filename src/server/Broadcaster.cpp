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

#include <utility>

#include "ingen/Interface.hpp"

#include "Broadcaster.hpp"
#include "PluginImpl.hpp"
#include "BlockFactory.hpp"

namespace Ingen {
namespace Server {

Broadcaster::Broadcaster()
	: _must_broadcast(false)
	, _bundle_depth(0)
{}

Broadcaster::~Broadcaster()
{
	std::lock_guard<std::mutex> lock(_clients_mutex);
	_clients.clear();
	_broadcastees.clear();
}

/** Register a client to receive messages over the notification band.
 */
void
Broadcaster::register_client(SPtr<Interface> client)
{
	std::lock_guard<std::mutex> lock(_clients_mutex);
	_clients.insert(client);
}

/** Remove a client from the list of registered clients.
 *
 * @return true if client was found and removed.
 */
bool
Broadcaster::unregister_client(SPtr<Interface> client)
{
	std::lock_guard<std::mutex> lock(_clients_mutex);
	const size_t erased = _clients.erase(client);
	_broadcastees.erase(client);
	return (erased > 0);
}

void
Broadcaster::set_broadcast(SPtr<Interface> client, bool broadcast)
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
	std::lock_guard<std::mutex> lock(_clients_mutex);
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
		const PluginImpl* const plugin = p.second;
		client->put(plugin->uri(), plugin->properties());
	}

	client->bundle_end();
}

} // namespace Server
} // namespace Ingen
