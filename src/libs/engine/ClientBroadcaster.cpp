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
#include "Patch.hpp"
#include "Node.hpp"
#include "Plugin.hpp"
#include "Port.hpp"
#include "Connection.hpp"
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
		cerr << "[ClientBroadcaster] Failed to find client: " << uri << endl;
		return NULL;
	}
}


void
ClientBroadcaster::send_error(const string& msg)
{
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->error(msg);
}

void
ClientBroadcaster::send_plugins_to(ClientInterface* client, const list<Plugin*>& plugin_list)
{
#if 0
	// FIXME: This probably isn't actually thread safe
	const list<Plugin*> plugs = plugin_list; // make a copy

	const Plugin* plugin;

	// FIXME FIXME FIXME
	OSCClientSender* osc_client = dynamic_cast<OSCClientSender*>(client);
	assert(osc_client);

	lo_timetag tt;
	lo_timetag_now(&tt);
	lo_bundle b = lo_bundle_new(tt);
	lo_message m = lo_message_new();
	list<lo_message> msgs;

	lo_message_add_int32(m, plugs.size());
	lo_bundle_add_message(b, "/om/num_plugins", m);
	msgs.push_back(m);

	for (list<Plugin*>::const_iterator j = plugs.begin(); j != plugs.end(); ++j) {
		plugin = (*j);
		m = lo_message_new();

		lo_message_add_string(m, plugin->type_string());
		lo_message_add_string(m, plugin->uri().c_str());
		lo_message_add_string(m, plugin->name().c_str());
		lo_bundle_add_message(b, "/om/plugin", m);
		msgs.push_back(m);
		if (lo_bundle_length(b) > 1024) {
			lo_send_bundle(osc_client->address(), b);
			lo_bundle_free(b);
			b = lo_bundle_new(tt);
		}
	}
	
	if (lo_bundle_length(b) > 0) {
		lo_send_bundle(osc_client->address(), b);
		lo_bundle_free(b);
	} else {
		lo_bundle_free(b);
	}
	for (list<lo_bundle>::const_iterator i = msgs.begin(); i != msgs.end(); ++i)
		lo_message_free(*i);
#endif
	client->transfer_begin();

	for (list<Plugin*>::const_iterator i = plugin_list.begin(); i != plugin_list.end(); ++i) {
		const Plugin* const plugin = *i;
		client->new_plugin(plugin->uri(), plugin->type_uri(), plugin->name());
	}

	client->transfer_end();
}


void
ClientBroadcaster::send_plugins(const list<Plugin*>& plugin_list)
{
	for (Clients::const_iterator c = _clients.begin(); c != _clients.end(); ++c)
		send_plugins_to((*c).second, plugin_list);
}


void
ClientBroadcaster::send_node(const Node* node, bool recursive)
{
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		ObjectSender::send_node((*i).second, node, recursive);
}


void
ClientBroadcaster::send_port(const Port* port)
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
ClientBroadcaster::send_patch_cleared(const string& patch_path)
{
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->patch_cleared(patch_path);
}

void
ClientBroadcaster::send_connection(const Connection* const c)
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


/** Send notification of a metadata update.
 *
 * Like control changes, does not send update to client that set the metadata, if applicable.
 */
void
ClientBroadcaster::send_metadata_update(const string& node_path, const string& key, const Atom& value)
{
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->metadata_update(node_path, key, value);
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
ClientBroadcaster::send_patch(const Patch* const p, bool recursive)
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


/** Sends all GraphObjects known to the engine.
 */
void
ClientBroadcaster::send_all_objects()
{	
	cerr << "FIXME: send_all" << endl;

	//for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
	//	(*i).second->send_all_objects();
}


} // namespace Ingen
