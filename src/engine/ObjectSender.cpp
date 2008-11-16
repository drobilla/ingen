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

#include "ObjectSender.hpp"
#include "interface/ClientInterface.hpp"
#include "EngineStore.hpp"
#include "PatchImpl.hpp"
#include "NodeImpl.hpp"
#include "PortImpl.hpp"
#include "PortImpl.hpp"
#include "ConnectionImpl.hpp"
#include "NodeFactory.hpp"
#include "interface/DataType.hpp"
#include "AudioBuffer.hpp"

namespace Ingen {


void
ObjectSender::send_object(ClientInterface* client, const GraphObjectImpl* object, bool recursive)
{
	client->new_object(object);

	const PatchImpl* patch = dynamic_cast<const PatchImpl*>(object);
	if (patch) {
		send_patch(client, patch, recursive);
		return;
	}
	
	const NodeImpl* node = dynamic_cast<const NodeImpl*>(object);
	if (node) {
		send_node(client, node, recursive);
		return;
	}

	const PortImpl* port = dynamic_cast<const PortImpl*>(object);
	if (port) {
		send_port(client, port);
		return;
	}
}


void
ObjectSender::send_patch(ClientInterface* client, const PatchImpl* patch, bool recursive)
{
	client->bundle_begin();

	client->new_patch(patch->path(), patch->internal_polyphony());
	client->set_property(patch->path(), "ingen:polyphonic", patch->polyphonic());
	
	// Send variable
	const GraphObjectImpl::Variables& data = patch->variables();
	for (GraphObjectImpl::Variables::const_iterator j = data.begin(); j != data.end(); ++j)
		client->set_variable(patch->path(), (*j).first, (*j).second);
	
	client->set_property(patch->path(), "ingen:enabled", (bool)patch->enabled());

	client->bundle_end();
	
	if (recursive) {

		// Send nodes
		for (List<NodeImpl*>::const_iterator j = patch->nodes().begin();
				j != patch->nodes().end(); ++j) {
			const NodeImpl* const node = (*j); 
			send_node(client, node, true);
		}

		// Send ports
		for (uint32_t i=0; i < patch->num_ports(); ++i) {
			PortImpl* const port = patch->port_impl(i);
			send_port(client, port);
		}

		// Send connections
		client->transfer_begin();
		for (PatchImpl::Connections::const_iterator j = patch->connections().begin();
				j != patch->connections().end(); ++j)
			client->connect((*j)->src_port_path(), (*j)->dst_port_path());
		client->transfer_end();
	}
}


/** Sends a node or a patch */
void
ObjectSender::send_node(ClientInterface* client, const NodeImpl* node, bool recursive)
{
	PluginImpl* const plugin = node->plugin_impl();

	assert(node->path().length() > 0);
	
	if (plugin->type() == Plugin::Patch) {
		send_patch(client, (PatchImpl*)node, recursive);
		return;
	}

	if (plugin->uri().length() == 0) {
		cerr << "Node " << node->path() << " plugin has no URI!  Not sending." << endl;
		return;
	}
	
	client->bundle_begin();
	
	client->new_node(node->path(), node->plugin()->uri());
	client->set_property(node->path(), "ingen:polyphonic", node->polyphonic());
	
	// Send variables
	const GraphObjectImpl::Variables& data = node->variables();
	for (GraphObjectImpl::Variables::const_iterator j = data.begin(); j != data.end(); ++j)
		client->set_variable(node->path(), (*j).first, (*j).second);
	
	// Send properties
	const GraphObjectImpl::Properties& prop = node->properties();
	for (GraphObjectImpl::Properties::const_iterator j = prop.begin(); j != prop.end(); ++j)
		client->set_property(node->path(), (*j).first, (*j).second);
	
	client->bundle_end();
	
	if (recursive) {
		// Send ports
		for (size_t j=0; j < node->num_ports(); ++j)
			send_port(client, node->port_impl(j));
	}
}


void
ObjectSender::send_port(ClientInterface* client, const PortImpl* port)
{
	assert(port);
	
	client->bundle_begin();

	client->new_port(port->path(), port->type().uri(), port->index(), port->is_output());
	client->set_property(port->path(), "ingen:polyphonic", port->polyphonic());
	
	// Send variable
	const GraphObjectImpl::Variables& data = port->variables();
	for (GraphObjectImpl::Variables::const_iterator j = data.begin(); j != data.end(); ++j)
		client->set_variable(port->path(), (*j).first, (*j).second);
	
	// Send properties
	const GraphObjectImpl::Properties& prop = port->properties();
	for (GraphObjectImpl::Properties::const_iterator j = prop.begin(); j != prop.end(); ++j)
		client->set_property(port->path(), (*j).first, (*j).second);
	
	// Send control value
	if (port->type() == DataType::CONTROL) {
		const Sample value = dynamic_cast<const AudioBuffer*>(port->buffer(0))->value_at(0);
		//cerr << port->path() << " sending default value " << default_value << endl;
		client->set_port_value(port->path(), value);
	}
	
	client->bundle_end();
}


} // namespace Ingen

