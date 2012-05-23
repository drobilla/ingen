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
#include <glibmm/thread.h>

#include "ingen/Interface.hpp"
#include "raul/log.hpp"

#include "Broadcaster.hpp"
#include "PluginImpl.hpp"
#include "NodeFactory.hpp"

#define LOG(s) (s("[Broadcaster] "))

namespace Ingen {
namespace Server {

/** Register a client to receive messages over the notification band.
 */
void
Broadcaster::register_client(const Raul::URI&     uri,
                             SharedPtr<Interface> client)
{
	Glib::Mutex::Lock lock(_clients_mutex);
	LOG(Raul::info)(Raul::fmt("Registered client <%1%>\n") % uri);
	_clients[uri] = client;
}

/** Remove a client from the list of registered clients.
 *
 * @return true if client was found and removed.
 */
bool
Broadcaster::unregister_client(const Raul::URI& uri)
{
	Glib::Mutex::Lock lock(_clients_mutex);
	const size_t erased = _clients.erase(uri);

	if (erased > 0) {
		LOG(Raul::info)(Raul::fmt("Unregistered client <%1%>\n") % uri);
	}

	return (erased > 0);
}

/** Looks up the client with the given source @a uri (which is used as the
 * unique identifier for registered clients).
 */
SharedPtr<Interface>
Broadcaster::client(const Raul::URI& uri)
{
	Glib::Mutex::Lock lock(_clients_mutex);
	Clients::iterator i = _clients.find(uri);
	if (i != _clients.end()) {
		return (*i).second;
	} else {
		return SharedPtr<Interface>();
	}
}

void
Broadcaster::send_plugins(const NodeFactory::Plugins& plugins)
{
	Glib::Mutex::Lock lock(_clients_mutex);
	for (Clients::const_iterator c = _clients.begin(); c != _clients.end(); ++c) {
		send_plugins_to((*c).second.get(), plugins);
	}
}

void
Broadcaster::send_plugins_to(Interface*                  client,
                             const NodeFactory::Plugins& plugins)
{
	client->bundle_begin();

	for (NodeFactory::Plugins::const_iterator i = plugins.begin(); i != plugins.end(); ++i) {
		const PluginImpl* const plugin = i->second;
		client->put(plugin->uri(), plugin->properties());
	}

	client->bundle_end();
}

} // namespace Server
} // namespace Ingen
