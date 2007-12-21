/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#ifndef PLUGINUI_H
#define PLUGINUI_H

#include <slv2/slv2.h>
#include <raul/SharedPtr.hpp>

namespace Ingen {
namespace Shared { class EngineInterface; }
namespace Client {

class NodeModel;


/** Model for a plugin available for loading.
 *
 * \ingroup IngenClient
 */
class PluginUI {
public:
	~PluginUI();

	static SharedPtr<PluginUI>
	create(SharedPtr<Shared::EngineInterface> engine,
	       SharedPtr<NodeModel>               node,
	       SLV2Plugin                         plugin);

	SharedPtr<Shared::EngineInterface> engine()   { return _engine; }
	SharedPtr<NodeModel>               node()     { return _node; }
	SLV2UIInstance                     instance() { return _instance; }

private:
	PluginUI(SharedPtr<Ingen::Shared::EngineInterface> engine,
	         SharedPtr<NodeModel>                      node);

	void set_instance(SLV2UIInstance instance) { _instance = instance; }

	SharedPtr<Shared::EngineInterface> _engine;
	SharedPtr<NodeModel>               _node;
	SLV2UIInstance                     _instance;
};


} // namespace Client
} // namespace Ingen

#endif // PLUGINUI_H


