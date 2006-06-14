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
#include "PortInfo.h"
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
		Node* const node = (*j);
		Port* const port = node->as_port(); // NULL unless a bridge node
		send_node(client, node);
		
		usleep(100);

		// If this is a bridge (input/output) node, send the patch control value as well
		if (port && port->port_info()->is_control())
			client->control_change(port->path(),
				((PortBase<sample>*)port)->buffer(0)->value_at(0));
	}
	
	for (List<Connection*>::const_iterator j = patch->connections().begin();
			j != patch->connections().end(); ++j)
		client->connection((*j)->src_port()->path(), (*j)->dst_port()->path());
	
	// Send port information
	/*for (size_t i=0; i < m_ports.size(); ++i) {
		Port* const port = m_ports.at(i);

		// Send metadata
		const map<string, string>& data = port->metadata();
		for (map<string, string>::const_iterator i = data.begin(); i != data.end(); ++i)
			om->client_broadcaster()->send_metadata_update_to(client, port->path(), (*i).first, (*i).second);
		
		if (port->port_info()->is_control())
			om->client_broadcaster()->send_control_change_to(client, port->path(),
				((PortBase<sample>*)port)->buffer(0)->value_at(0));
	}*/
}


void
ObjectSender::send_node(ClientInterface* client, const Node* node)
{
	int polyphonic = 
		(node->poly() > 1
		&& node->poly() == node->parent_patch()->internal_poly()
		? 1 : 0);

	assert(node->path().length() > 0);
	if (node->plugin()->uri().length() == 0) {
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
		Port* const     port = ports.at(j);
		PortInfo* const info = port->port_info();

		assert(port);
		assert(info);
		
		client->new_port(port->path(), info->type_string(), info->is_output());

		/*m = lo_message_new();
		lo_message_add_string(m, port->path().c_str());
		lo_message_add_string(m, info->type_string().c_str());
		lo_message_add_string(m, info->direction_string().c_str());
		lo_message_add_string(m, info->hint_string().c_str());
		lo_message_add_float(m, info->default_val());
		lo_message_add_float(m, info->min_val());
		lo_message_add_float(m, info->max_val());
		lo_bundle_add_message(b, "/om/new_port", m);
		msgs.push_back(m);*/
		
		// If the bundle is getting very large, send it and start
		// a new one
		/*if (lo_bundle_length(b) > 1024) {
		  lo_send_bundle(_address, b);
		  lo_bundle_free(b);
		  b = lo_bundle_new(tt);
		}*/
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
	PortInfo* info = port->port_info();
	
	client->new_port(port->path(), info->type_string(), info->is_output());
	
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

