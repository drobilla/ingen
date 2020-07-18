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

#ifndef INGEN_CLIENT_PLUGINUI_HPP
#define INGEN_CLIENT_PLUGINUI_HPP

#include "signal.hpp"

#include "ingen/LV2Features.hpp"
#include "ingen/Resource.hpp"
#include "ingen/ingen.h"
#include "ingen/types.hpp"
#include "lilv/lilv.h"
#include "suil/suil.h"

#include <cstdint>
#include <set>

namespace ingen {

class Atom;
class URI;
class World;

namespace client {

class BlockModel;

/** Custom user interface for a plugin.
 *
 * @ingroup IngenClient
 */
class INGEN_API PluginUI {
public:
	~PluginUI();

	/** Create a UI for the given block and plugin.
	 *
	 * This does not actually instantiate the UI itself, so signals can be
	 * connected first.  The caller should connect to signal_property_changed,
	 * then call instantiate().
	 */
	static SPtr<PluginUI> create(ingen::World&          world,
	                             SPtr<const BlockModel> block,
	                             const LilvPlugin*      plugin);

	/** Instantiate the UI.
	 *
	 * If true is returned, instantiation was successfull and the widget can be
	 * obtained with get_widget(). Otherwise, instantiation failed, so there is
	 * no widget and the UI can not be used.
	 */
	bool instantiate();
	bool instantiated () { return _instance != nullptr; }

	SuilWidget get_widget();

	void port_event(uint32_t    port_index,
	                uint32_t    buffer_size,
	                uint32_t    format,
	                const void* buffer);

	bool is_resizable() const;

	/** Signal emitted when the UI sets a property.
	 *
	 * The application must connect to this signal to communicate with the
	 * engine and/or update itself as necessary.
	 */
	INGEN_SIGNAL(property_changed, void,
	             const URI&,        // Subject
	             const URI&,        // Predicate
	             const Atom&,       // Object
	             Resource::Graph)   // Context

	ingen::World&          world() const { return _world; }
	SPtr<const BlockModel> block() const { return _block; }

private:
	PluginUI(ingen::World&          world,
	         SPtr<const BlockModel> block,
	         LilvUIs*               uis,
	         const LilvUI*          ui,
	         const LilvNode*        ui_type);

	ingen::World&          _world;
	SPtr<const BlockModel> _block;
	SuilInstance*          _instance;
	LilvUIs*               _uis;
	const LilvUI*          _ui;
	LilvNode*              _ui_node;
	LilvNode*              _ui_type;
	std::set<uint32_t>     _subscribed_ports;

	static SuilHost* ui_host;

	SPtr<LV2Features::FeatureArray> _features;
};

} // namespace client
} // namespace ingen

#endif // INGEN_CLIENT_PLUGINUI_HPP
