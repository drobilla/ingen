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

#include "ObjectSender.hpp"
#include "interface/ClientInterface.hpp"
#include "EngineStore.hpp"
#include "PatchImpl.hpp"
#include "NodeImpl.hpp"
#include "PortImpl.hpp"
#include "ConnectionImpl.hpp"
#include "NodeFactory.hpp"
#include "interface/DataType.hpp"
#include "AudioBuffer.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {

using namespace Shared;

void
ObjectSender::send_object(ClientInterface* client, const GraphObjectImpl* object, bool recursive)
{
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
ObjectSender::send_patch(ClientInterface* client, const PatchImpl* patch, bool recursive, bool bundle)
{
	if (bundle)
		client->transfer_begin();

	client->put(patch->meta_uri(), patch->meta().properties());
	client->put(patch->path(),     patch->properties());

	if (recursive) {
		// Send nodes
		for (List<NodeImpl*>::const_iterator j = patch->nodes().begin();
				j != patch->nodes().end(); ++j) {
			const NodeImpl* const node = (*j);
			send_node(client, node, true, false);
		}

		// Send ports
		for (uint32_t i=0; i < patch->num_ports(); ++i) {
			PortImpl* const port = patch->port_impl(i);
			send_port(client, port, false);
		}

		// Send connections
		for (PatchImpl::Connections::const_iterator j = patch->connections().begin();
				j != patch->connections().end(); ++j)
			client->connect((*j)->src_port_path(), (*j)->dst_port_path());
	}

	if (bundle)
		client->transfer_end();
}


/** Sends a node or a patch */
void
ObjectSender::send_node(ClientInterface* client, const NodeImpl* node, bool recursive, bool bundle)
{
	PluginImpl* const plugin = node->plugin_impl();

	if (plugin->type() == Plugin::Patch) {
		send_patch(client, (PatchImpl*)node, recursive);
		return;
	}

	if (plugin->uri().length() == 0) {
		cerr << "Node " << node->path() << " plugin has no URI!  Not sending." << endl;
		return;
	}

	if (bundle)
		client->transfer_begin();

	client->put(node->path(), node->properties());

	if (recursive) {
		// Send ports
		for (size_t j=0; j < node->num_ports(); ++j)
			send_port(client, node->port_impl(j), false);
	}

	if (bundle)
		client->transfer_end();
}


void
ObjectSender::send_port(ClientInterface* client, const PortImpl* port, bool bundle)
{
	assert(port);

	if (bundle)
		client->bundle_begin();

	PatchImpl* graph_parent = dynamic_cast<PatchImpl*>(port->parent_node());

	if (graph_parent)
		client->put(port->meta_uri(), port->meta().properties());

	client->put(port->path(), port->properties());

	if (graph_parent && graph_parent->internal_polyphony() > 1)
		client->set_property(port->meta_uri(), "ingen:polyphonic", bool(port->polyphonic()));

	// Send control value
	if (port->type() == DataType::CONTROL) {
		const Sample& value = PtrCast<const AudioBuffer>(port->buffer(0))->value_at(0);
		client->set_port_value(port->path(), value);
	}

	if (bundle)
		client->bundle_end();
}


} // namespace Ingen

