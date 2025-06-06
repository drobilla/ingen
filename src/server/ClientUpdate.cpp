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

#include "ClientUpdate.hpp"

#include "BlockImpl.hpp"
#include "BufferFactory.hpp"
#include "GraphImpl.hpp"
#include "PluginImpl.hpp"
#include "PortImpl.hpp"
#include "PortType.hpp"

#include <ingen/Arc.hpp>
#include <ingen/Forge.hpp>
#include <ingen/Interface.hpp>
#include <ingen/Properties.hpp>
#include <ingen/Resource.hpp>
#include <ingen/URI.hpp>
#include <ingen/URIs.hpp>
#include <raul/Path.hpp>

#include <boost/intrusive/slist.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <utility>

namespace ingen::server {

void
ClientUpdate::put(const URI&        uri,
                  const Properties& props,
                  Resource::Graph   ctx)
{
	const ClientUpdate::Put put = { uri, props, ctx };
	puts.push_back(put);
}

void
ClientUpdate::put_port(const PortImpl* port)
{
	const URIs& uris = port->bufs().uris();
	if (port->is_a(PortType::CONTROL) || port->is_a(PortType::CV)) {
		Properties props = port->properties();
		props.erase(uris.ingen_value);
		props.emplace(uris.ingen_value, port->value());
		put(port->uri(), props);
	} else {
		put(port->uri(), port->properties());
	}
}

void
ClientUpdate::put_block(const BlockImpl* block)
{
	const PluginImpl* const plugin = block->plugin_impl();
	const URIs&             uris   = plugin->uris();

	if (uris.ingen_Graph == plugin->type()) {
		put_graph(static_cast<const GraphImpl*>(block));
	} else {
		put(block->uri(), block->properties());
		for (size_t j = 0; j < block->num_ports(); ++j) {
			put_port(block->port_impl(j));
		}
	}
}

void
ClientUpdate::put_graph(const GraphImpl* graph)
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
		const auto    arc     = a.second;
		const Connect connect = {arc->tail_path(), arc->head_path()};
		connects.push_back(connect);
	}
}

void
ClientUpdate::put_plugin(PluginImpl* plugin)
{
	put(plugin->uri(), plugin->properties());

	for (const auto& p : plugin->presets()) {
		put_preset(plugin->uris(), plugin->uri(), p.first, p.second);
	}
}

void
ClientUpdate::put_preset(const URIs&        uris,
                         const URI&         plugin,
                         const URI&         preset,
                         const std::string& label)
{
	const Properties props{
		{ uris.rdf_type, uris.pset_Preset.urid_atom() },
		{ uris.rdfs_label, uris.forge.alloc(label) },
		{ uris.lv2_appliesTo, uris.forge.make_urid(plugin) }};
	put(preset, props);
}

void
ClientUpdate::del(const URI& subject)
{
	dels.push_back(subject);
}

namespace {

/** Returns true if a is closer to the root than b. */
inline bool
put_higher_than(const ClientUpdate::Put& a, const ClientUpdate::Put& b)
{
	return (std::count(a.uri.begin(), a.uri.end(), '/') <
	        std::count(b.uri.begin(), b.uri.end(), '/'));
}

} // namespace

void
ClientUpdate::send(Interface& dest)
{
	// Send deletions
	for (const URI& subject : dels) {
		dest.del(subject);
	}

	// Send puts in increasing depth order so parents are sent first
	std::stable_sort(puts.begin(), puts.end(), put_higher_than);
	for (const ClientUpdate::Put& put : puts) {
		dest.put(put.uri, put.properties, put.ctx);
	}

	// Send connections
	for (const ClientUpdate::Connect& connect : connects) {
		dest.connect(connect.tail, connect.head);
	}
}

} // namespace ingen::server
