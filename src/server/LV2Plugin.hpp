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

#include "PluginImpl.hpp"

#include "ingen/URI.hpp"
#include "lilv/lilv.h"
#include "raul/Symbol.hpp"

namespace ingen {

class World;

namespace server {

class BlockImpl;
class BufferFactory;
class Engine;
class GraphImpl;

/** Implementation of an LV2 plugin (loaded shared library).
 */
class LV2Plugin : public PluginImpl
{
public:
	LV2Plugin(World& world, const LilvPlugin* lplugin);

	BlockImpl* instantiate(BufferFactory&      bufs,
	                       const raul::Symbol& symbol,
	                       bool                polyphonic,
	                       GraphImpl*          parent,
	                       Engine&             engine,
	                       const LilvState*    state) override;

	raul::Symbol symbol() const override;

	World&            world()       const { return _world; }
	const LilvPlugin* lilv_plugin() const { return _lilv_plugin; }

	void update_properties() override;

	void load_presets() override;

	URI bundle_uri() const override {
		const LilvNode* bundle = lilv_plugin_get_bundle_uri(_lilv_plugin);
		return URI(lilv_node_as_uri(bundle));
	}

private:
	World&            _world;
	const LilvPlugin* _lilv_plugin;
};

} // namespace server
} // namespace ingen

#endif // INGEN_ENGINE_LV2PLUGIN_HPP
