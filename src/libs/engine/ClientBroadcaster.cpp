/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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

#include "ClientBroadcaster.h"
#include <cassert>
#include <iostream>
#include <unistd.h>
#include "Engine.h"
#include "ObjectStore.h"
#include "NodeFactory.h"
#include "util.h"
#include "Patch.h"
#include "Node.h"
#include "Plugin.h"
#include "TypedPort.h"
#include "Connection.h"
#include "AudioDriver.h"
#include "ObjectSender.h"
#include "interface/ClientKey.h"
#include "interface/ClientInterface.h"
#include "OSCClient.h"
using std::cout; using std::cerr; using std::endl;
using Ingen::Shared::ClientInterface;

namespace Ingen {

	
/** Register a client to receive messages over the notification band.
 */
void
ClientBroadcaster::register_client(const ClientKey key, CountedPtr<ClientInterface> client)
{
	assert(key.type() == ClientKey::OSC_URL);
	assert(key.uri() != "");

	bool found = false;
	for (ClientList::iterator i = _clients.begin(); i != _clients.end(); ++i)
		if ((*i).first == key)
			found = true;

	if (!found) {
		_clients.push_back(pair<ClientKey, CountedPtr<ClientInterface> >(key, client));
		cout << "[ClientBroadcaster] Registered client " << key.uri()
			<< " (" << _clients.size() << " clients)" << endl;
	} else {
		cout << "[ClientBroadcaster] Client already registered." << endl;
	}
}


/** Remove a client from the list of registered clients.
 *
 * The removed client is returned (not deleted).  It is the caller's
 * responsibility to delete the returned pointer, if it's not NULL.
 *
 * @return true if client was found and removed (and refcount decremented).
 */
bool
ClientBroadcaster::unregister_client(const ClientKey& key)
{
	cerr << "FIXME: unregister broken\n";
	return false;

#if 0
	if (responder == NULL)
		return NULL;

	// FIXME: remove filthy cast
	const string url = lo_address_get_url(((OSCResponder*)responder)->source());
	ClientInterface* r = NULL;

	for (ClientList::iterator i = _clients.begin(); i != _clients.end(); ++i) {
		if ((*i).second->url() == url) {
			r = *i;
			_clients.erase(i);
			break;
		}
	}
	
	if (r != NULL)
		cout << "[OSC] Unregistered client " << r->url() << " (" << _clients.size() << " clients)" << endl;
	else
		cerr << "[OSC] ERROR: Unable to find client to unregister!" << endl;
	
	return r;
#endif
}



/** Looks up the client with the given @a source address (which is used as the
 * unique identifier for registered clients).
 *
 * (A responder is passed to remove the dependency on liblo addresses in request
 * events, in anticipation of libom and multiple ways of responding to clients).
 */
CountedPtr<ClientInterface>
ClientBroadcaster::client(const ClientKey& key)
{
	for (ClientList::iterator i = _clients.begin(); i != _clients.end(); ++i)
		if ((*i).first == key)
			return (*i).second;

	cerr << "[ClientBroadcaster] Failed to find client." << endl;

	return NULL;
}


void
ClientBroadcaster::send_error(const string& msg)
{
	for (ClientList::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->error(msg);
}


/* FIXME: Make a copy method for list and just make a copy and pass it here
 * instead of this global+locking mess */
void
ClientBroadcaster::send_plugins_to(ClientInterface* client)
{
	Engine::instance().node_factory()->lock_plugin_list();
	
	const list<Plugin*>& plugs = Engine::instance().node_factory()->plugins();
	const Plugin* plugin;

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
			// FIXME FIXME FIXME dirty, dirty cast
			lo_send_bundle(((OSCClient*)client)->address(), b);
			lo_bundle_free(b);
			b = lo_bundle_new(tt);
		}
	}
	
	if (lo_bundle_length(b) > 0) {
		// FIXME FIXME FIXME dirty, dirty cast
		lo_send_bundle(((OSCClient*)client)->address(), b);
		lo_bundle_free(b);
	} else {
		lo_bundle_free(b);
	}
	for (list<lo_bundle>::const_iterator i = msgs.begin(); i != msgs.end(); ++i)
		lo_message_free(*i);

	Engine::instance().node_factory()->unlock_plugin_list();
}


void
ClientBroadcaster::send_node(const Node* node)
{
	for (ClientList::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		ObjectSender::send_node((*i).second.get(), node);
}


void
ClientBroadcaster::send_port(const Port* port)
{
	for (ClientList::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		ObjectSender::send_port((*i).second.get(), port);
}


void
ClientBroadcaster::send_destroyed(const string& path)
{
	assert(path != "/");
	for (ClientList::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->object_destroyed(path);
}

void
ClientBroadcaster::send_patch_cleared(const string& patch_path)
{
	for (ClientList::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->patch_cleared(patch_path);
}

void
ClientBroadcaster::send_connection(const Connection* const c)
{
	for (ClientList::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->connection(c->src_port()->path(), c->dst_port()->path());
}


void
ClientBroadcaster::send_disconnection(const string& src_port_path, const string& dst_port_path)
{
	for (ClientList::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->disconnection(src_port_path, dst_port_path);
}


void
ClientBroadcaster::send_patch_enable(const string& patch_path)
{
	for (ClientList::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->patch_enabled(patch_path);
}


void
ClientBroadcaster::send_patch_disable(const string& patch_path)
{
	for (ClientList::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->patch_disabled(patch_path);
}


/** Send notification of a metadata update.
 *
 * Like control changes, does not send update to client that set the metadata, if applicable.
 */
void
ClientBroadcaster::send_metadata_update(const string& node_path, const string& key, const string& value)
{
	for (ClientList::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
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
	for (ClientList::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->control_change(port_path, value);
}


void
ClientBroadcaster::send_program_add(const string& node_path, int bank, int program, const string& name)
{
	for (ClientList::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->program_add(node_path, bank, program, name);
}


void
ClientBroadcaster::send_program_remove(const string& node_path, int bank, int program)
{
	for (ClientList::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->program_remove(node_path, bank, program);
}


/** Send a patch.
 *
 * Sends all objects underneath Patch - contained Nodes, etc.
 */
void
ClientBroadcaster::send_patch(const Patch* const p)
{
	for (ClientList::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		ObjectSender::send_patch((*i).second.get(), p);
}


/** Sends notification of an GraphObject's renaming
 */
void
ClientBroadcaster::send_rename(const string& old_path, const string& new_path)
{
	for (ClientList::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		(*i).second->object_renamed(old_path, new_path);
}


/** Sends all GraphObjects known to the engine.
 */
void
ClientBroadcaster::send_all_objects()
{	
	cerr << "FIXME: send_all" << endl;

	//for (ClientList::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
	//	(*i).second->send_all_objects();
}

/*
void
ClientBroadcaster::send_node_creation_messages(const Node* const node)
{
	// This is pretty stupid :/  in and out and back again!
	for (ClientList::const_iterator i = _clients.begin(); i != _clients.end(); ++i)
		node->send_creation_messages((*i).second);
}
*/	

} // namespace Ingen
