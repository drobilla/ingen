/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "ObjectSender.hpp"
#include "ingen/Interface.hpp"
#include "ingen/shared/LV2URIMap.hpp"
#include "ingen/shared/URIs.hpp"
#include "EngineStore.hpp"
#include "PatchImpl.hpp"
#include "NodeImpl.hpp"
#include "PortImpl.hpp"
#include "ConnectionImpl.hpp"
#include "NodeFactory.hpp"
#include "PortType.hpp"
#include "AudioBuffer.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Server {

void
ObjectSender::send_object(Interface*             client,
                          const GraphObjectImpl* object,
                          bool                   recursive)
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
ObjectSender::send_patch(Interface* client, const PatchImpl* patch, bool recursive, bool bundle)
{
	if (bundle)
		client->bundle_begin();

	client->put(patch->path(),
	            patch->properties(Resource::INTERNAL),
	            Resource::INTERNAL);

	client->put(patch->path(),
	            patch->properties(Resource::EXTERNAL),
	            Resource::EXTERNAL);

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
			client->connect(j->second->src_port_path(), j->second->dst_port_path());
	}

	if (bundle)
		client->bundle_end();
}

/** Sends a node or a patch */
void
ObjectSender::send_node(Interface* client, const NodeImpl* node, bool recursive, bool bundle)
{
	PluginImpl* const plugin = node->plugin_impl();

	if (plugin->type() == Plugin::Patch) {
		send_patch(client, (PatchImpl*)node, recursive);
		return;
	}

	if (plugin->uri().length() == 0) {
		error << "Node " << node->path() << "'s plugin has no URI!  Not sending." << endl;
		return;
	}

	if (bundle)
		client->bundle_begin();

	client->put(node->path(), node->properties());

	if (recursive) {
		// Send ports
		for (size_t j=0; j < node->num_ports(); ++j)
			send_port(client, node->port_impl(j), false);
	}

	if (bundle)
		client->bundle_end();
}

void
ObjectSender::send_port(Interface* client, const PortImpl* port, bool bundle)
{
	assert(port);

	if (bundle)
		client->bundle_begin();

	if (port->is_a(PortType::CONTROL) || port->is_a(PortType::CV)) {
		Resource::Properties props = port->properties();
		props.erase(port->bufs().uris().ingen_value);
		props.insert(make_pair(port->bufs().uris().ingen_value,
		                       port->value()));
		client->put(port->path(), props);
	} else {
		client->put(port->path(), port->properties());
	}

	if (bundle)
		client->bundle_end();
}

} // namespace Server
} // namespace Ingen

