/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ObjectSender.h"
#include "interface/ClientInterface.h"
#include "Om.h"
#include "OmApp.h"
#include "ObjectStore.h"
#include "Patch.h"
#include "Node.h"
#include "Port.h"
#include "PortBase.h"
#include "Connection.h"
#include "NodeFactory.h"

namespace Om {

/** Send all currently existing objects to client.
 */
void
ObjectSender::send_all(ClientInterface* client)
{
	for (Tree<OmObject*>::iterator i = om->object_store()->objects().begin();
			i != om->object_store()->objects().end(); ++i)
		if ((*i)->as_patch() != NULL && (*i)->parent() == NULL)
			send_patch(client, (*i)->as_patch());
			//(*i)->as_node()->send_creation_messages(client);

}


void
ObjectSender::send_patch(ClientInterface* client, const Patch* patch)
{
	client->new_patch(patch->path(), patch->internal_poly());
	
	for (List<Node*>::const_iterator j = patch->nodes().begin();
			j != patch->nodes().end(); ++j) {
		const Node* const node = (*j);

		send_node(client, node);
	}
	
	for (List<Connection*>::const_iterator j = patch->connections().begin();
			j != patch->connections().end(); ++j)
		client->connection((*j)->src_port()->path(), (*j)->dst_port()->path());
	
	// Send port information
	for (size_t i=0; i < patch->num_ports(); ++i) {
		Port* const port = patch->ports().at(i);

		// Send metadata
		const map<string, string>& data = port->metadata();
		for (map<string, string>::const_iterator i = data.begin(); i != data.end(); ++i)
			client->metadata_update(port->path(), (*i).first, (*i).second);
		
		// Control port, send value
		if (port->type() == DataType::FLOAT && port->buffer_size() == 1)
			client->control_change(port->path(), ((PortBase<sample>*)port)->buffer(0)->value_at(0));
	}
	
	// Send metadata
	const map<string, string>& data = patch->metadata();
	for (map<string, string>::const_iterator j = data.begin(); j != data.end(); ++j)
		client->metadata_update(patch->path(), (*j).first, (*j).second);

}


/** Sends a node or a patch */
void
ObjectSender::send_node(ClientInterface* client, const Node* node)
{
	// Don't send node notification for bridge nodes, from the client's
	// perspective they don't even exist (just the ports they represent)
	// FIXME: hack, these nodes probably shouldn't even exist in the
	// engine anymore
	if (const_cast<Node*>(node)->as_port()) { // bridge node if as_port() returns non-NULL
		// FIXME: remove this whole thing.  shouldn't be any bridge nodes anymore
		assert(false);
		send_port(client, const_cast<Node*>(node)->as_port());
		return;
	}

	const Plugin* const plugin = node->plugin();

	int polyphonic = 
		(node->poly() > 1
		&& node->poly() == node->parent_patch()->internal_poly()
		? 1 : 0);

	assert(node->path().length() > 0);
	
	if (plugin->type() == Plugin::Patch) {
		send_patch(client, (Patch*)node);
		return;
	}

	if (plugin->uri().length() == 0) {
		cerr << "Node " << node->path() << " plugin has no URI!  Not sending." << endl;
		return;
	}

	
	client->bundle_begin();

	// FIXME: bundleify
	
	const Array<Port*>& ports = node->ports();

	client->new_node(node->plugin()->type_string(), node->plugin()->uri(),
	                 node->path(), polyphonic, ports.size());
	
	// Send ports
	for (size_t j=0; j < ports.size(); ++j) {
		Port* const port = ports.at(j);
		assert(port);
		
		send_port(client, port);
		//client->new_port(port->path(), port->type().uri(), port->is_output());
	}

	client->bundle_end();
	
	// Send metadata
	const map<string, string>& data = node->metadata();
	for (map<string, string>::const_iterator j = data.begin(); j != data.end(); ++j)
		client->metadata_update(node->path(), (*j).first, (*j).second);
}


void
ObjectSender::send_port(ClientInterface* client, const Port* port)
{
	assert(port);

	// FIXME: temporary compatibility hack
	string type = port->type().uri();
	if (port->type() == DataType::FLOAT) {
		if (port->buffer_size() == 1) 
			type = "CONTROL";
		else
			type = "AUDIO";
	}
		
	client->new_port(port->path(), type, port->is_output());
	
	// Send metadata
	const map<string, string>& data = port->metadata();
	for (map<string, string>::const_iterator j = data.begin(); j != data.end(); ++j)
		client->metadata_update(port->path(), (*j).first, (*j).second);
}


void
ObjectSender::send_plugins(ClientInterface* client)
{
	om->node_factory()->lock_plugin_list();
	
	const list<Plugin*>& plugs = om->node_factory()->plugins();

/*
	lo_timetag tt;
	lo_timetag_now(&tt);
	lo_bundle b = lo_bundle_new(tt);
	lo_message m = lo_message_new();
	list<lo_message> msgs;

	lo_message_add_int32(m, plugs.size());
	lo_bundle_add_message(b, "/om/num_plugins", m);
	msgs.push_back(m);
*/
	for (list<Plugin*>::const_iterator j = plugs.begin(); j != plugs.end(); ++j) {
		const Plugin* const p = *j;
		client->new_plugin(p->type_string(), p->uri(), p->name());
	}
/*
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
	}*/
/*	
	if (lo_bundle_length(b) > 0) {
		// FIXME FIXME FIXME dirty, dirty cast
		lo_send_bundle(((OSCClient*)client)->address(), b);
		lo_bundle_free(b);
	} else {
		lo_bundle_free(b);
	}
	for (list<lo_bundle>::const_iterator i = msgs.begin(); i != msgs.end(); ++i)
		lo_message_free(*i);
*/
	om->node_factory()->unlock_plugin_list();
}


} // namespace Om

