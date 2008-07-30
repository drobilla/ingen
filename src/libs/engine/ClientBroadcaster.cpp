/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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
#include <iostream>
#include <unistd.h>
#include "interface/ClientInterface.hpp"
#include "ClientBroadcaster.hpp"
#include "ObjectStore.hpp"
#include "NodeFactory.hpp"
#include "util.hpp"
#include "PatchImpl.hpp"
#include "NodeImpl.hpp"
#include "PluginImpl.hpp"
#include "PortImpl.hpp"
#include "ConnectionImpl.hpp"
#include "AudioDriver.hpp"
#include "ObjectSender.hpp"
#include "OSCClientSender.hpp"

using namespace std;
using Ingen::Shared::ClientInterface;

namespace Ingen {

	
/** Register a client to receive messages over the notification band.
 */
void
ClientBroadcaster::register_client(const string& uri, ClientInterface* client)
{
	Clients::iterator i = _clients.find(uri);

	if (i == _clients.end()) {
		_clients[uri] = client;
		cout << "[ClientBroadcaster] Registered client: " << uri << endl;
	} else {
		cout << "[ClientBroadcaster] Client already registered: " << uri << endl;
	}
}


/** Remove a client from the list of registered clients.
 *
 * @return true if client was found and removed.
 */
bool
ClientBroadcaster::unregister_client(const string& uri)
{
	size_t erased = _clients.erase(uri);
	
	if (erased > 0)
		cout << "Unregistered client: " << uri << endl;
	else
		cout << "Failed to find client to unregister: " << uri << endl;

	return (erased > 0);
}



/** Looks up the client with the given @a source address (which is used as the
 * unique identifier for registered clients).
 *
 * (A responder is passed to remove the dependency on liblo addresses in request
 * events, in anticipation of libom and multiple ways of responding to clients).
 */
ClientInterface*
ClientBroadcaster::client(const string& uri)
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
ClientBroadcaster::send_error(const string& msg)
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
		client->new_plugin(plugin->uri(), plugin->type_uri(), plugin->symbol(), plugin->name());
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
ClientBroadcaster::send_node(const NodeImpl* node, bool recursive)
{
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		ObjectSender::send_node((*i).second, node, recursive);
}


void
ClientBroadcaster::send_port(const PortImpl* port)
{
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		ObjectSender::send_port((*i).second, port);
}


void
ClientBroadcaster::send_destroyed(const string& path)
{
	assert(path != "/");
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->object_destroyed(path);
}


void
ClientBroadcaster::send_polyphonic(const string& path, bool polyphonic)
{
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->polyphonic(path, polyphonic);
}


void
ClientBroadcaster::send_patch_cleared(const string& patch_path)
{
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->patch_cleared(patch_path);
}

void
ClientBroadcaster::send_connection(const SharedPtr<const ConnectionImpl> c)
{
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->connection(c->src_port()->path(), c->dst_port()->path());
}


void
ClientBroadcaster::send_disconnection(const string& src_port_path, const string& dst_port_path)
{
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->disconnection(src_port_path, dst_port_path);
}


void
ClientBroadcaster::send_patch_enable(const string& patch_path)
{
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->patch_enabled(patch_path);
}


void
ClientBroadcaster::send_patch_disable(const string& patch_path)
{
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->patch_disabled(patch_path);
}


void
ClientBroadcaster::send_patch_polyphony(const string& patch_path, uint32_t poly)
{
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->patch_polyphony(patch_path, poly);
}


/** Send notification of a variable update.
 *
 * Like control changes, does not send update to client that set the variable, if applicable.
 */
void
ClientBroadcaster::send_variable_change(const string& node_path, const string& key, const Atom& value)
{
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->variable_change(node_path, key, value);
}


/** Send notification of a control change.
 *
 * If responder is specified, the notification will not be send to the address of
 * that responder (to avoid sending redundant information back to clients and
 * forcing clients to ignore things to avoid feedback loops etc).
 */
void
ClientBroadcaster::send_control_change(const string& port_path, float value)
{
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->control_change(port_path, value);
}


void
ClientBroadcaster::send_port_activity(const string& port_path)
{
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->port_activity(port_path);
}


void
ClientBroadcaster::send_program_add(const string& node_path, int bank, int program, const string& name)
{
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->program_add(node_path, bank, program, name);
}


void
ClientBroadcaster::send_program_remove(const string& node_path, int bank, int program)
{
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->program_remove(node_path, bank, program);
}


/** Send a patch.
 *
 * Sends all objects underneath Patch - contained Nodes, etc.
 */
void
ClientBroadcaster::send_patch(const PatchImpl* p, bool recursive)
{
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		ObjectSender::send_patch((*i).second, p, recursive);
}


/** Sends notification of an GraphObject's renaming
 */
void
ClientBroadcaster::send_rename(const string& old_path, const string& new_path)
{
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->object_renamed(old_path, new_path);
}


} // namespace Ingen
