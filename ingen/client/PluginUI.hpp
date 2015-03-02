/*
  This file is part of Ingen.
  Copyright 2007-2013 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_CLIENT_PLUGINUI_HPP
#define INGEN_CLIENT_PLUGINUI_HPP

#include <set>

#include "ingen/LV2Features.hpp"
#include "ingen/ingen.h"
#include "ingen/types.hpp"
#include "lilv/lilv.h"
#include "suil/suil.h"

namespace Ingen {

class Interface;
class World;

namespace Client {

class BlockModel;

/** Model for a plugin available for loading.
 *
 * @ingroup IngenClient
 */
class INGEN_API PluginUI {
public:
	~PluginUI();

	static SPtr<PluginUI> create(Ingen::World*          world,
	                             SPtr<const BlockModel> block,
	                             const LilvPlugin*      plugin);

	SuilWidget get_widget();

	void port_event(uint32_t    port_index,
	                uint32_t    buffer_size,
	                uint32_t    format,
	                const void* buffer);

	bool is_resizable() const;

	Ingen::World*          world() const { return _world; }
	SPtr<const BlockModel> block() const { return _block; }

private:
	PluginUI(Ingen::World*          world,
	         SPtr<const BlockModel> block,
	         const LilvNode*        ui_node);

	Ingen::World*          _world;
	SPtr<const BlockModel> _block;
	SuilInstance*          _instance;
	LilvNode*              _ui_node;
	std::set<uint32_t>     _subscribed_ports;

	static SuilHost* ui_host;

	SPtr<LV2Features::FeatureArray> _features;
};

} // namespace Client
} // namespace Ingen

#endif // INGEN_CLIENT_PLUGINUI_HPP
