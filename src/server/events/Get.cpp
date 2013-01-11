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
#include "ingen/Node.hpp"
#include "ingen/Store.hpp"

#include "BlockImpl.hpp"
#include "Broadcaster.hpp"
#include "BufferFactory.hpp"
#include "Driver.hpp"
#include "Engine.hpp"
#include "Get.hpp"
#include "GraphImpl.hpp"
#include "PluginImpl.hpp"
#include "PortImpl.hpp"

namespace Ingen {
namespace Server {
namespace Events {

static void
send_graph(Interface* client, const GraphImpl* graph);

Get::Get(Engine&              engine,
         SharedPtr<Interface> client,
         int32_t              id,
         SampleCount          timestamp,
         const Raul::URI&     uri)
	: Event(engine, client, id, timestamp)
	, _uri(uri)
	, _object(NULL)
	, _plugin(NULL)
	, _lock(engine.store()->lock(), Glib::NOT_LOCK)
{
}

bool
Get::pre_process()
{
	_lock.acquire();

	if (_uri == "ingen:/plugins") {
		_plugins = _engine.block_factory()->plugins();
		return Event::pre_process_done(Status::SUCCESS);
	} else if (_uri == "ingen:/engine") {
		return Event::pre_process_done(Status::SUCCESS);
	} else if (Node::uri_is_path(_uri)) {
		_object = _engine.store()->get(Node::uri_to_path(_uri));
		return Event::pre_process_done(
			_object ? Status::SUCCESS : Status::NOT_FOUND, _uri);
	} else {
		_plugin = _engine.block_factory()->plugin(_uri);
		return Event::pre_process_done(
			_plugin ? Status::SUCCESS : Status::NOT_FOUND, _uri);
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
		client->put(port->uri(), props);
	} else {
		client->put(port->uri(), port->properties());
	}
}

static void
send_block(Interface* client, const BlockImpl* block)
{
	PluginImpl* const plugin = block->plugin_impl();
	if (plugin->type() == Plugin::Graph) {
		send_graph(client, (const GraphImpl*)block);
	} else {
		client->put(block->uri(), block->properties());
		for (size_t j = 0; j < block->num_ports(); ++j) {
			send_port(client, block->port_impl(j));
		}
	}
}

static void
send_graph(Interface* client, const GraphImpl* graph)
{
	client->put(graph->uri(),
	            graph->properties(Resource::Graph::INTERNAL),
	            Resource::Graph::INTERNAL);

	client->put(graph->uri(),
	            graph->properties(Resource::Graph::EXTERNAL),
	            Resource::Graph::EXTERNAL);

	// Send blocks
	for (const auto& b : graph->blocks()) {
		send_block(client, &b);
	}

	// Send ports
	for (uint32_t i = 0; i < graph->num_ports_non_rt(); ++i) {
		send_port(client, graph->port_impl(i));
	}

	// Send arcs
	for (const auto& a : graph->arcs()) {
		client->connect(a.second->tail_path(), a.second->head_path());
	}
}

void
Get::post_process()
{
	Broadcaster::Transfer t(*_engine.broadcaster());
	if (respond() == Status::SUCCESS && _request_client) {
		if (_uri == "ingen:/plugins") {
			_engine.broadcaster()->send_plugins_to(_request_client.get(), _plugins);
		} else if (_uri == "ingen:/engine") {
			// TODO: Keep a proper RDF model of the engine
			URIs& uris = _engine.world()->uris();
			_request_client->set_property(
				uris.ingen_engine,
				uris.ingen_sampleRate,
				uris.forge.make(int32_t(_engine.driver()->sample_rate())));
		} else if (_object) {
			const BlockImpl* block = NULL;
			const GraphImpl* graph = NULL;
			const PortImpl*  port  = NULL;
			if ((graph = dynamic_cast<const GraphImpl*>(_object))) {
				send_graph(_request_client.get(), graph);
			} else if ((block = dynamic_cast<const BlockImpl*>(_object))) {
				send_block(_request_client.get(), block);
			} else if ((port = dynamic_cast<const PortImpl*>(_object))) {
				send_port(_request_client.get(), port);
			}
		} else if (_plugin) {
			_request_client->put(_uri, _plugin->properties());
		}
	}
	_lock.release();
}

} // namespace Events
} // namespace Server
} // namespace Ingen

