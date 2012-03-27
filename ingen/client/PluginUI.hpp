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


#ifndef INGEN_CLIENT_PLUGINUI_HPP
#define INGEN_CLIENT_PLUGINUI_HPP

#include "raul/SharedPtr.hpp"

#include "lilv/lilv.h"

#include "suil/suil.h"

#include "ingen/shared/LV2Features.hpp"

namespace Ingen {

class Interface;

namespace Shared { class World; }

namespace Client {

class NodeModel;

/** Model for a plugin available for loading.
 *
 * \ingroup IngenClient
 */
class PluginUI {
public:
	~PluginUI();

	static SharedPtr<PluginUI> create(Ingen::Shared::World*      world,
	                                  SharedPtr<const NodeModel> node,
	                                  const LilvPlugin*          plugin);

	SuilWidget get_widget();

	void port_event(uint32_t    port_index,
	                uint32_t    buffer_size,
	                uint32_t    format,
	                const void* buffer);

	Ingen::Shared::World*       world() const { return _world; }
	SharedPtr<const NodeModel>  node()  const { return _node; }

private:
	PluginUI(Ingen::Shared::World*       world,
	         SharedPtr<const NodeModel>  node);

	Ingen::Shared::World*      _world;
	SharedPtr<const NodeModel> _node;
	SuilInstance*              _instance;

	static SuilHost* ui_host;

	SharedPtr<Shared::LV2Features::FeatureArray> _features;
};

} // namespace Client
} // namespace Ingen

#endif // INGEN_CLIENT_PLUGINUI_HPP

