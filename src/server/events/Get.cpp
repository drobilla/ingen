/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

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

void
Get::Response::put(const Raul::URI&            uri,
                   const Resource::Properties& props,
                   Resource::Graph             ctx)
{
	const Get::Response::Put put = { uri, props, ctx };
	puts.push_back(put);
}

void
Get::Response::put_port(const PortImpl* port)
{
	if (port->is_a(PortType::CONTROL) || port->is_a(PortType::CV)) {
		Resource::Properties props = port->properties();
		props.erase(port->bufs().uris().ingen_value);
		props.insert(std::make_pair(port->bufs().uris().ingen_value,
		                            port->value()));
		put(port->uri(), props);
	} else {
		put(port->uri(), port->properties());
	}
}

void
Get::Response::put_block(const BlockImpl* block)
{
	const PluginImpl* const plugin = block->plugin_impl();
	const URIs&             uris   = plugin->uris();

	if (uris.ingen_Graph == plugin->type()) {
		put_graph((const GraphImpl*)block);
	} else {
		put(block->uri(), block->properties());
		for (size_t j = 0; j < block->num_ports(); ++j) {
			put_port(block->port_impl(j));
		}
	}
}

void
Get::Response::put_graph(const GraphImpl* graph)
{
	put(graph->uri(),
	    graph->properties(Resource::Graph::INTERNAL),
	    Resource::Graph::INTERNAL);

	put(graph->uri(),
	    graph->properties(Resource::Graph::EXTERNAL),
	    Resource::Graph::EXTERNAL);

	// Enqueue blocks
	for (const auto& b : graph->blocks()) {
		put_block(&b);
	}

	// Enqueue ports
	for (uint32_t i = 0; i < graph->num_ports_non_rt(); ++i) {
		put_port(graph->port_impl(i));
	}

	// Enqueue arcs
	for (const auto& a : graph->arcs()) {
		const SPtr<const Arc> arc     = a.second;
		const Connect         connect = { arc->tail_path(), arc->head_path() };
		connects.push_back(connect);
	}
}

void
Get::Response::put_plugin(PluginImpl* plugin)
{
	put(plugin->uri(), plugin->properties());

	for (const auto& p : plugin->presets()) {
		put_preset(plugin->uris(), plugin->uri(), p.first, p.second);
	}
}

void
Get::Response::put_preset(const URIs&        uris,
                          const Raul::URI&   plugin,
                          const Raul::URI&   preset,
                          const std::string& label)
{
	Resource::Properties props{
		{ uris.rdf_type, uris.pset_Preset.urid },
		{ uris.rdfs_label, uris.forge.alloc(label) },
		{ uris.lv2_appliesTo, uris.forge.make_urid(plugin) }};
	put(preset, props);
}

/** Returns true if a is closer to the root than b. */
static inline bool
put_higher_than(const Get::Response::Put& a, const Get::Response::Put& b)
{
	return (std::count(a.uri.begin(), a.uri.end(), '/') <
	        std::count(b.uri.begin(), b.uri.end(), '/'));
}

void
Get::Response::send(Interface* dest)
{
	// Sort puts by increasing depth so parents are sent first
	std::stable_sort(puts.begin(), puts.end(), put_higher_than);
	for (const Response::Put& put : puts) {
		dest->put(put.uri, put.properties, put.ctx);
	}
	for (const Response::Connect& connect : connects) {
		dest->connect(connect.tail, connect.head);
	}
}

Get::Get(Engine&          engine,
         SPtr<Interface>  client,
         int32_t          id,
         SampleCount      timestamp,
         const Raul::URI& uri)
	: Event(engine, client, id, timestamp)
	, _uri(uri)
	, _object(NULL)
	, _plugin(NULL)
{}

bool
Get::pre_process()
{
	std::unique_lock<std::mutex> lock(_engine.store()->mutex());

	if (_uri == "ingen:/plugins") {
		_plugins = _engine.block_factory()->plugins();
		return Event::pre_process_done(Status::SUCCESS);
	} else if (_uri == "ingen:/engine") {
		return Event::pre_process_done(Status::SUCCESS);
	} else if (Node::uri_is_path(_uri)) {
		if ((_object = _engine.store()->get(Node::uri_to_path(_uri)))) {
			const BlockImpl* block = NULL;
			const GraphImpl* graph = NULL;
			const PortImpl*  port  = NULL;
			if ((graph = dynamic_cast<const GraphImpl*>(_object))) {
				_response.put_graph(graph);
			} else if ((block = dynamic_cast<const BlockImpl*>(_object))) {
				_response.put_block(block);
			} else if ((port = dynamic_cast<const PortImpl*>(_object))) {
				_response.put_port(port);
			} else {
				return Event::pre_process_done(Status::BAD_OBJECT_TYPE, _uri);
			}
			return Event::pre_process_done(Status::SUCCESS);
		}
		return Event::pre_process_done(Status::NOT_FOUND, _uri);
	} else if ((_plugin = _engine.block_factory()->plugin(_uri))) {
		_response.put_plugin(_plugin);
		return Event::pre_process_done(Status::SUCCESS);
	} else {
		return Event::pre_process_done(Status::NOT_FOUND, _uri);
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
				Raul::URI("ingen:/engine"),
				uris.param_sampleRate,
				uris.forge.make(int32_t(_engine.driver()->sample_rate())));
		} else {
			_response.send(_request_client.get());
		}
	}
}

} // namespace Events
} // namespace Server
} // namespace Ingen
