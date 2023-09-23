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

#include "LV2Plugin.hpp"

#include "Engine.hpp"
#include "LV2Block.hpp"

#include "ingen/Forge.hpp"
#include "ingen/Log.hpp"
#include "ingen/URIs.hpp"
#include "ingen/World.hpp"
#include "lilv/lilv.h"
#include "raul/Symbol.hpp"

#include <cstdlib>
#include <string>

namespace ingen::server {

LV2Plugin::LV2Plugin(World& world, const LilvPlugin* lplugin)
	: PluginImpl(world.uris(),
	             world.uris().lv2_Plugin.urid_atom(),
	             URI(lilv_node_as_uri(lilv_plugin_get_uri(lplugin))))
	, _world(world)
	, _lilv_plugin(lplugin)
{
	set_property(_uris.rdf_type, _uris.lv2_Plugin);

	LV2Plugin::update_properties();
}

void
LV2Plugin::update_properties()
{
	LilvNode* minor = lilv_world_get(_world.lilv_world(),
	                                 lilv_plugin_get_uri(_lilv_plugin),
	                                 _uris.lv2_minorVersion,
	                                 nullptr);
	LilvNode* micro = lilv_world_get(_world.lilv_world(),
	                                 lilv_plugin_get_uri(_lilv_plugin),
	                                 _uris.lv2_microVersion,
	                                 nullptr);

	if (lilv_node_is_int(minor) && lilv_node_is_int(micro)) {
		set_property(_uris.lv2_minorVersion,
		             _world.forge().make(lilv_node_as_int(minor)));
		set_property(_uris.lv2_microVersion,
		             _world.forge().make(lilv_node_as_int(micro)));
	}

	lilv_node_free(minor);
	lilv_node_free(micro);
}

raul::Symbol
LV2Plugin::symbol() const
{
	std::string working = uri();
	if (working.back() == '/') {
		working = working.substr(0, working.length() - 1);
	}

	while (working.length() > 0) {
		const size_t last_slash = working.find_last_of('/');
		const std::string symbol = working.substr(last_slash+1);
		if ( (symbol[0] >= 'a' && symbol[0] <= 'z')
		     || (symbol[0] >= 'A' && symbol[0] <= 'Z') ) {
			return raul::Symbol::symbolify(symbol);
		}

		working = working.substr(0, last_slash);
	}

	return raul::Symbol("lv2_symbol");
}

BlockImpl*
LV2Plugin::instantiate(BufferFactory&      bufs,
                       const raul::Symbol& symbol,
                       bool                polyphonic,
                       GraphImpl*          parent,
                       Engine&             engine,
                       const LilvState*    state)
{
	auto* b = new LV2Block(
		this, symbol, polyphonic, parent, engine.sample_rate());

	if (!b->instantiate(bufs, state)) {
		delete b;
		return nullptr;
	}

	return b;
}

void
LV2Plugin::load_presets()
{
	const URIs& uris    = _world.uris();
	LilvWorld*  lworld  = _world.lilv_world();
	LilvNodes*  presets = lilv_plugin_get_related(_lilv_plugin, uris.pset_Preset);

	if (presets) {
		LILV_FOREACH (nodes, i, presets) {
			const LilvNode* preset = lilv_nodes_get(presets, i);
			lilv_world_load_resource(lworld, preset);

			LilvNodes* labels = lilv_world_find_nodes(
				lworld, preset, uris.rdfs_label, nullptr);
			if (labels) {
				const LilvNode* label = lilv_nodes_get_first(labels);

				_presets.emplace(URI(lilv_node_as_uri(preset)),
				                 lilv_node_as_string(label));

				lilv_nodes_free(labels);
			} else {
				_world.log().error(
					"Preset <%1%> has no rdfs:label\n",
					lilv_node_as_string(lilv_nodes_get(presets, i)));
			}
		}

		lilv_nodes_free(presets);
	}

	PluginImpl::load_presets();
}

} // namespace ingen::server
