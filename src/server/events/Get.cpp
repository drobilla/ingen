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

#include <utility>

#include "ingen/Interface.hpp"

#include "Broadcaster.hpp"
#include "Driver.hpp"
#include "Engine.hpp"
#include "EngineStore.hpp"
#include "Get.hpp"
#include "NodeImpl.hpp"
#include "PatchImpl.hpp"
#include "PluginImpl.hpp"
#include "PortImpl.hpp"

namespace Ingen {
namespace Server {
namespace Events {

static void
send_patch(Interface* client, const PatchImpl* patch);

Get::Get(Engine&              engine,
         SharedPtr<Interface> client,
         int32_t              id,
         SampleCount          timestamp,
         const Raul::URI&     uri)
	: Event(engine, client, id, timestamp)
	, _uri(uri)
	, _object(NULL)
	, _plugin(NULL)
	, _lock(engine.engine_store()->lock(), Glib::NOT_LOCK)
{
}

bool
Get::pre_process()
{
	_lock.acquire();

	if (_uri == "ingen:plugins") {
		_plugins = _engine.node_factory()->plugins();
		return Event::pre_process_done(SUCCESS);
	} else if (Raul::Path::is_valid(_uri.str())) {
		_object = _engine.engine_store()->find_object(Raul::Path(_uri.str()));
		return Event::pre_process_done(_object ? SUCCESS : NOT_FOUND);
	} else {
		_plugin = _engine.node_factory()->plugin(_uri);
		return Event::pre_process_done(_plugin ? SUCCESS : NOT_FOUND);
	}
}

static void
send_port(Interface* client, const PortImpl* port)
{
	if (port->is_a(PortType::CONTROL) || port->is_a(PortType::CV)) {
		Resource::Properties props = port->properties();
		props.erase(port->bufs().uris().ingen_value);
		props.insert(std::make_pair(port->bufs().uris().ingen_value,
		                            port->value()));
		client->put(port->path(), props);
	} else {
		client->put(port->path(), port->properties());
	}
}

static void
send_node(Interface* client, const NodeImpl* node)
{
	PluginImpl* const plugin = node->plugin_impl();
	if (plugin->type() == Plugin::Patch) {
		send_patch(client, (PatchImpl*)node);
	} else {
		client->put(node->path(), node->properties());
		for (size_t j = 0; j < node->num_ports(); ++j) {
			send_port(client, node->port_impl(j));
		}
	}
}

static void
send_patch(Interface* client, const PatchImpl* patch)
{
	client->put(patch->path(),
	            patch->properties(Resource::INTERNAL),
	            Resource::INTERNAL);

	client->put(patch->path(),
	            patch->properties(Resource::EXTERNAL),
	            Resource::EXTERNAL);

	// Send nodes
	for (Raul::List<NodeImpl*>::const_iterator j = patch->nodes().begin();
	     j != patch->nodes().end(); ++j) {
		const NodeImpl* const node = (*j);
		send_node(client, node);
	}

	// Send ports
	for (uint32_t i = 0; i < patch->num_ports_non_rt(); ++i) {
		PortImpl* const port = patch->port_impl(i);
		send_port(client, port);
	}

	// Send edges
	for (PatchImpl::Edges::const_iterator j = patch->edges().begin();
	     j != patch->edges().end(); ++j) {
		client->connect(j->second->tail_path(), j->second->head_path());
	}
}

void
Get::post_process()
{
	if (_uri == "ingen:plugins") {
		respond(SUCCESS);
		if (_request_client) {
			_engine.broadcaster()->send_plugins_to(_request_client.get(), _plugins);
		}
	} else if (_uri == "ingen:engine") {
		respond(SUCCESS);
		// TODO: Keep a proper RDF model of the engine
		if (_request_client) {
			Shared::URIs& uris = _engine.world()->uris();
			_request_client->set_property(
				uris.ingen_engine,
				uris.ingen_sampleRate,
				uris.forge.make(int32_t(_engine.driver()->sample_rate())));
		}
	} else if (!_object && !_plugin) {
		respond(NOT_FOUND);
	} else if (!_request_client) {
		respond(CLIENT_NOT_FOUND);
	} else {
		respond(SUCCESS);
		if (_object) {
			_request_client->bundle_begin();
			const NodeImpl*  node  = NULL;
			const PatchImpl* patch = NULL;
			const PortImpl*  port  = NULL;
			if ((patch = dynamic_cast<const PatchImpl*>(_object))) {
				send_patch(_request_client.get(), patch);
			} else if ((node = dynamic_cast<const NodeImpl*>(_object))) {
				send_node(_request_client.get(), node);
			} else if ((port = dynamic_cast<const PortImpl*>(_object))) {
				send_port(_request_client.get(), port);
			}
			_request_client->bundle_end();
		} else if (_plugin) {
			_request_client->put(_uri, _plugin->properties());
		}
		_lock.release();
	}
}

} // namespace Events
} // namespace Server
} // namespace Ingen

