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

#ifndef INGEN_ENGINE_LV2PLUGIN_HPP
#define INGEN_ENGINE_LV2PLUGIN_HPP

#include <cstdlib>

#include "ingen/types.hpp"
#include "lilv/lilv.h"

#include "PluginImpl.hpp"

namespace Ingen {

class World;

namespace Server {

class GraphImpl;
class BlockImpl;

/** Implementation of an LV2 plugin (loaded shared library).
 */
class LV2Plugin : public PluginImpl
{
public:
	LV2Plugin(World* world, const LilvPlugin* lplugin);

	BlockImpl* instantiate(BufferFactory&      bufs,
	                       const Raul::Symbol& symbol,
	                       bool                polyphonic,
	                       GraphImpl*          parent,
	                       Engine&             engine,
	                       const LilvState*    state);

	const Raul::Symbol symbol() const;

	World*            world()       const { return _world; }
	const LilvPlugin* lilv_plugin() const { return _lilv_plugin; }

	void update_properties();

	void load_presets();

	URI bundle_uri() const {
		const LilvNode* bundle = lilv_plugin_get_bundle_uri(_lilv_plugin);
		return URI(lilv_node_as_uri(bundle));
	}

private:
	World*            _world;
	const LilvPlugin* _lilv_plugin;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_LV2PLUGIN_HPP
