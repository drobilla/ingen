/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
 *
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef INGEN_CLIENT_PLUGINUI_HPP
#define INGEN_CLIENT_PLUGINUI_HPP

#include "raul/SharedPtr.hpp"
#include "slv2/slv2.h"

#include "LV2Features.hpp"

namespace Ingen {
namespace Shared { class EngineInterface; class World; }
namespace Client {

class NodeModel;

/** Model for a plugin available for loading.
 *
 * \ingroup IngenClient
 */
class PluginUI {
public:
	~PluginUI();

	static SharedPtr<PluginUI> create(Ingen::Shared::World* world,
	                                  SharedPtr<NodeModel>  node,
	                                  SLV2Plugin            plugin);

	LV2UI_Widget get_widget();

	void port_event(uint32_t    port_index,
	                uint32_t    buffer_size,
	                uint32_t    format,
	                const void* buffer);

	Ingen::Shared::World* world() const { return _world; }
	SharedPtr<NodeModel>  node()  const { return _node; }

private:
	PluginUI(Ingen::Shared::World* world,
	         SharedPtr<NodeModel>  node);

	Ingen::Shared::World* _world;
	SharedPtr<NodeModel>  _node;
	SLV2UIInstance        _instance;

	static SLV2UIHost ui_host;

    SharedPtr<Shared::LV2Features::FeatureArray> _features;
};

} // namespace Client
} // namespace Ingen

#endif // INGEN_CLIENT_PLUGINUI_HPP

