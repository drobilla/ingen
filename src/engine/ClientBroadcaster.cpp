/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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



/** Looks up the client with the given @a source address (which is used as the
 * unique identifier for registered clients).
 *
 * (A responder is passed to remove the dependency on liblo addresses in request
 * events, in anticipation of libom and multiple ways of responding to clients).
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
ClientBroadcaster::bundle_begin()
{
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->bundle_begin();
}


void
ClientBroadcaster::bundle_end()
{
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->bundle_end();
}


void
ClientBroadcaster::transfer_begin()
{
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->transfer_begin();
}


void
ClientBroadcaster::transfer_end()
{
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->transfer_end();
}


void
ClientBroadcaster::error(const string& msg)
{
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->error(msg);
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


void
ClientBroadcaster::send_plugins(const NodeFactory::Plugins& plugins)
{
	for (Clients::const_iterator c = _clients.begin(); c != _clients.end(); ++c)
		send_plugins_to((*c).second, plugins);
}


void
ClientBroadcaster::del(const Path& path)
{
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->del(path);
}


void
ClientBroadcaster::connect(const Path& src_port_path, const Path& dst_port_path)
{
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->connect(src_port_path, dst_port_path);
}


void
ClientBroadcaster::disconnect(const Path& src_port_path, const Path& dst_port_path)
{
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->disconnect(src_port_path, dst_port_path);
}


void
ClientBroadcaster::put(const Raul::URI& subject, const Shared::Resource::Properties& properties)
{
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->put(subject, properties);
}


/** Send notification of a property update.
 *
 * Like control changes, does not send update to client that set the property, if applicable.
 */
void
ClientBroadcaster::set_property(const URI& subject, const URI& key, const Atom& value)
{
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->set_property(subject, key, value);
}


/** Send notification of a control change.
 *
 * If responder is specified, the notification will not be send to the address of
 * that responder (to avoid sending redundant information back to clients and
 * forcing clients to ignore things to avoid feedback loops etc).
 */
void
ClientBroadcaster::set_port_value(const Path& port_path, const Raul::Atom& value)
{
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->set_port_value(port_path, value);
}


void
ClientBroadcaster::set_voice_value(const Path& port_path, uint32_t voice, const Raul::Atom& value)
{
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->set_voice_value(port_path, voice, value);
}


void
ClientBroadcaster::activity(const Path& path)
{
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->activity(path);
}


/** Send an object.
 *
 * @param p         Object to send
 * @param recursive If true send all children of object
 */
void
ClientBroadcaster::send_object(const GraphObjectImpl* p, bool recursive)
{
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		ObjectSender::send_object((*i).second, p, recursive);
}


/** Sends notification of an GraphObject's renaming
 */
void
ClientBroadcaster::move(const Path& old_path, const Path& new_path)
{
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->move(old_path, new_path);
}


} // namespace Ingen
