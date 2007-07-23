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

#include "ObjectSender.h"
#include "interface/ClientInterface.h"
#include "ObjectStore.h"
#include "Patch.h"
#include "Node.h"
#include "Port.h"
#include "Port.h"
#include "Connection.h"
#include "NodeFactory.h"
#include "DataType.h"
#include "AudioBuffer.h"

namespace Ingen {


void
ObjectSender::send_patch(ClientInterface* client, const Patch* patch, bool recursive)
{
	client->new_patch(patch->path(), patch->internal_poly());
	
	if (recursive) {

		// Send nodes
		for (Raul::List<Node*>::const_iterator j = patch->nodes().begin();
				j != patch->nodes().end(); ++j) {

			const Node* const node = (*j); 
			send_node(client, node, true);
		}

		// Send ports
		for (size_t i=0; i < patch->num_ports(); ++i) {

			Port* const port = patch->ports().at(i);
			send_port(client, port);

		}

		// Send connections
		for (Raul::List<Connection*>::const_iterator j = patch->connections().begin();
				j != patch->connections().end(); ++j) {
			
			client->connection((*j)->src_port()->path(), (*j)->dst_port()->path());

		}

	}

	// Send metadata
	const GraphObject::MetadataMap& data = patch->metadata();
	for (GraphObject::MetadataMap::const_iterator j = data.begin(); j != data.end(); ++j)
		client->metadata_update(patch->path(), (*j).first, (*j).second);
	
	if (patch->enabled())
		client->patch_enabled(patch->path());
}


/** Sends a node or a patch */
void
ObjectSender::send_node(ClientInterface* client, const Node* node, bool recursive)
{
	const Plugin* const plugin = node->plugin();

	int polyphonic = 
		(node->poly() > 1
		&& node->poly() == node->parent_patch()->internal_poly()
		? 1 : 0);

	assert(node->path().length() > 0);
	
	if (plugin->type() == Plugin::Patch) {
		send_patch(client, (Patch*)node, recursive);
		return;
	}

	if (plugin->uri().length() == 0) {
		cerr << "Node " << node->path() << " plugin has no URI!  Not sending." << endl;
		return;
	}
	
	client->bundle_begin();
	
	client->new_node(node->plugin()->uri(), node->path(), polyphonic, node->ports().size());
	
	// Send metadata
	const GraphObject::MetadataMap& data = node->metadata();
	for (GraphObject::MetadataMap::const_iterator j = data.begin(); j != data.end(); ++j)
		client->metadata_update(node->path(), (*j).first, (*j).second);
	
	client->bundle_end();
	
	if (recursive) {
		// Send ports
		for (size_t j=0; j < node->ports().size(); ++j)
			send_port(client, node->ports().at(j));
	}
}


void
ObjectSender::send_port(ClientInterface* client, const Port* port)
{
	assert(port);
	
	//cerr << "Sending port " << port->path();

	// FIXME: temporary compatibility hack
	string type = port->type().uri();
	if (port->type() == DataType::FLOAT) {
		if (port->buffer_size() == 1) 
			type = "ingen:control";
		else
			type = "ingen:audio";
	} else if (port->type() == DataType::MIDI) {
		type = "ingen:midi";
	} else if (port->type() == DataType::OSC) {
		type = "ingen:osc";
	}
	
	//cerr << ", type = " << type << endl;

	client->bundle_begin();

	client->new_port(port->path(), type, port->is_output());
	
	// Send control value
	if (port->type() == DataType::FLOAT && port->buffer_size() == 1) {
		const Sample value = dynamic_cast<const AudioBuffer*>(port->buffer(0))->value_at(0);
		//cerr << port->path() << " sending default value " << default_value << endl;
		client->control_change(port->path(), value);
	}
	
	// Send metadata
	const GraphObject::MetadataMap& data = port->metadata();
	for (GraphObject::MetadataMap::const_iterator j = data.begin(); j != data.end(); ++j)
		client->metadata_update(port->path(), (*j).first, (*j).second);
	
	client->bundle_end();
}


void
ObjectSender::send_plugins(ClientInterface* client, const list<Plugin*>& plugs)
{
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
		client->new_plugin(p->uri(), p->type_uri(), p->name());
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
}


} // namespace Ingen

