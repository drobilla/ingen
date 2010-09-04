/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
 *
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <cassert>
#include <unistd.h>
#include "raul/log.hpp"
#include "interface/ClientInterface.hpp"
#include "ClientBroadcaster.hpp"
#include "PluginImpl.hpp"
#include "ConnectionImpl.hpp"
#include "EngineStore.hpp"
#include "ObjectSender.hpp"
#include "util.hpp"

#define LOG(s) s << "[ClientBroadcaster] "

using namespace std;
using namespace Raul;
using Ingen::Shared::ClientInterface;

namespace Ingen {


/** Register a client to receive messages over the notification band.
 */
void
ClientBroadcaster::register_client(const URI& uri, ClientInterface* client)
{
	Clients::iterator i = _clients.find(uri);

	if (i == _clients.end()) {
		_clients[uri] = client;
		LOG(info) << "Registered client: " << uri << endl;
	} else {
		LOG(warn) << "Client already registered: " << uri << endl;
	}
}


/** Remove a client from the list of registered clients.
 *
 * @return true if client was found and removed.
 */
bool
ClientBroadcaster::unregister_client(const URI& uri)
{
	size_t erased = _clients.erase(uri);

	if (erased > 0)
		LOG(info) << "Unregistered client: " << uri << endl;
	else
		LOG(warn) << "Failed to find client to unregister: " << uri << endl;

	return (erased > 0);
}



/** Looks up the client with the given source @a uri (which is used as the
 * unique identifier for registered clients).
 */
ClientInterface*
ClientBroadcaster::client(const URI& uri)
{
	Clients::iterator i = _clients.find(uri);
	if (i != _clients.end()) {
		return (*i).second;
	} else {
		return NULL;
	}
}


void
ClientBroadcaster::send_plugins(const NodeFactory::Plugins& plugins)
{
	for (Clients::const_iterator c = _clients.begin(); c != _clients.end(); ++c)
		send_plugins_to((*c).second, plugins);
}


void
ClientBroadcaster::send_plugins_to(ClientInterface* client, const NodeFactory::Plugins& plugins)
{
	client->transfer_begin();

	for (NodeFactory::Plugins::const_iterator i = plugins.begin(); i != plugins.end(); ++i) {
		const PluginImpl* const plugin = i->second;
		client->put(plugin->uri(), plugin->properties());
	}

	client->transfer_end();
}


/** Send an object to all clients.
 *
 * @param o         Object to send
 * @param recursive If true send all children of object
 */
void
ClientBroadcaster::send_object(const GraphObjectImpl* o, bool recursive)
{
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		ObjectSender::send_object((*i).second, o, recursive);
}


} // namespace Ingen
