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

#include <string>

#include "ingen/Log.hpp"
#include "ingen/URIs.hpp"
#include "lv2/lv2plug.in/ns/ext/presets/presets.h"

#include "Driver.hpp"
#include "Engine.hpp"
#include "LV2Block.hpp"
#include "LV2Plugin.hpp"

using namespace std;

namespace Ingen {
namespace Server {

LV2Plugin::LV2Plugin(SPtr<LV2Info> lv2_info, const Raul::URI& uri)
	: PluginImpl(lv2_info->world().uris(),
	             lv2_info->world().uris().lv2_Plugin.urid,
	             uri)
	, _lilv_plugin(NULL)
	, _lv2_info(lv2_info)
{
	set_property(_uris.rdf_type, _uris.lv2_Plugin);
}

const Raul::Symbol
LV2Plugin::symbol() const
{
	string working = uri();
	if (working[working.length() - 1] == '/')
		working = working.substr(0, working.length() - 1);

	while (working.length() > 0) {
		size_t last_slash = working.find_last_of("/");
		const string symbol = working.substr(last_slash+1);
		if ( (symbol[0] >= 'a' && symbol[0] <= 'z')
		     || (symbol[0] >= 'A' && symbol[0] <= 'Z') )
			return Raul::Symbol::symbolify(symbol);
		else
			working = working.substr(0, last_slash);
	}

	return Raul::Symbol("lv2_symbol");
}

BlockImpl*
LV2Plugin::instantiate(BufferFactory&      bufs,
                       const Raul::Symbol& symbol,
                       bool                polyphonic,
                       GraphImpl*          parent,
                       Engine&             engine)
{
	LV2Block* b = new LV2Block(
		this, symbol, polyphonic, parent, engine.driver()->sample_rate());

	if (!b->instantiate(bufs)) {
		delete b;
		return NULL;
	} else {
		return b;
	}
}

void
LV2Plugin::lilv_plugin(const LilvPlugin* p)
{
	_lilv_plugin = p;
}

void
LV2Plugin::load_presets()
{
	LilvWorld* lworld      = _lv2_info->world().lilv_world();
	LilvNode*  pset_Preset = lilv_new_uri(lworld, LV2_PRESETS__Preset);
	LilvNode*  rdfs_label  = lilv_new_uri(lworld, LILV_NS_RDFS "label");
	LilvNodes* presets     = lilv_plugin_get_related(_lilv_plugin, pset_Preset);

	if (presets) {
		LILV_FOREACH(nodes, i, presets) {
			const LilvNode* preset = lilv_nodes_get(presets, i);
			lilv_world_load_resource(lworld, preset);

			LilvNodes* labels = lilv_world_find_nodes(
				lworld, preset, rdfs_label, NULL);
			if (labels) {
				const LilvNode* label = lilv_nodes_get_first(labels);

				_presets.emplace(Raul::URI(lilv_node_as_uri(preset)),
				                 lilv_node_as_string(label));

				lilv_nodes_free(labels);
			} else {
				_lv2_info->world().log().error(
					fmt("Preset <%1%> has no rdfs:label\n")
					% lilv_node_as_string(lilv_nodes_get(presets, i)));
			}
		}

		lilv_nodes_free(presets);
	}

	lilv_node_free(rdfs_label);
	lilv_node_free(pset_Preset);

	PluginImpl::load_presets();
}

} // namespace Server
} // namespace Ingen
