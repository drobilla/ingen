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

#include <sstream>
#include <raul/Path.hpp>
#include "PluginModel.hpp"
#include "PatchModel.hpp"

using namespace std;
using Ingen::Shared::EngineInterface;

namespace Ingen {
namespace Client {

#ifdef HAVE_SLV2
SLV2World   PluginModel::_slv2_world   = NULL;
SLV2Plugins PluginModel::_slv2_plugins = NULL;
#endif

Raul::RDF::World* PluginModel::_rdf_world = NULL;
	

string
PluginModel::default_node_name(SharedPtr<PatchModel> parent)
{
	string default_name = Raul::Path::nameify(_name);
	string name;

	char num_buf[3];
	for (uint i=0; i < 99; ++i) {
		name = default_name;
		if (i != 0) {
			snprintf(num_buf, 3, "%d", i+1);
			name += num_buf;
		}
		if (!parent->get_node(name))
			break;
	}

	return name;
}


struct NodeController {
	EngineInterface* engine;
	NodeModel*       node;
};


void
lv2_ui_write(LV2UI_Controller controller,
             uint32_t         port,
             uint32_t         buffer_size,
             const void*      buffer)
{
	NodeController* nc = (NodeController*)controller;

	nc->engine->set_port_value_immediate(nc->node->ports()[port]->path(),
			"ingen:midi", buffer_size, buffer);
}


void
lv2_ui_command(LV2UI_Controller   controller,
               uint32_t           argc,
               const char* const* argv)
{
	cerr << "********* LV2 UI COMMAND" << endl;
}

	
void
lv2_ui_program_change(LV2UI_Controller controller,
                      unsigned char    program)
{
	cerr << "********* LV2 UI PROGRAM CHANGE" << endl;
}

	
void
lv2_ui_program_save(LV2UI_Controller controller,
                    unsigned char    program,
                    const char*      name)
{
	cerr << "********* LV2 UI PROGRAM SAVE" << endl;
}

	
#ifdef HAVE_SLV2
SLV2UIInstance
PluginModel::ui(EngineInterface* engine, NodeModel* node) const
{
	if (_type != LV2)
		return NULL;

	Glib::Mutex::Lock(_rdf_world->mutex());

	// FIXME: leak
	NodeController* controller = new NodeController();
	controller->engine = engine;
	controller->node = node;
	
	SLV2UIInstance ret = NULL;
			
	const char* gtk_gui_uri = "http://ll-plugins.nongnu.org/lv2/ext/gui/dev/1#GtkGUI";

	SLV2UIs uis = slv2_plugin_get_uis(_slv2_plugin);
	SLV2UI ui = NULL;

	if (slv2_values_size(uis) > 0) {
		for (unsigned i=0; i < slv2_uis_size(uis); ++i) {
			ui = slv2_uis_get_at(uis, i);

			if (slv2_ui_is_type(ui, gtk_gui_uri)) {
				break;
			} else {
				ui = NULL;
			}
		}
	}

	if (ui) {
		cout << "Found GTK Plugin UI " << slv2_ui_get_uri(ui) << endl;
	
		ret = slv2_ui_instantiate(_slv2_plugin,
				ui,
				lv2_ui_write, lv2_ui_command,
				lv2_ui_program_change, lv2_ui_program_save,
				controller, NULL);
	
		//slv2_ui_free(ui);
	}

	return ret;
}

#endif

} // namespace Client
} // namespace Ingen
