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
#include "lv2_osc_print.h"
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
	string default_name = Raul::Path::nameify(_symbol);
	string name;

	char num_buf[3];
	for (uint i=0; i < 99; ++i) {
		name = default_name;
		if (i != 0) {
			snprintf(num_buf, 4, "_%d", i+1);
			name += num_buf;
		}
		if (!parent->find_child(name))
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
             uint32_t         port_index,
             uint32_t         buffer_size,
             const void*      buffer)
{
	/*cerr << "********* LV2 UI WRITE:" << endl;
	lv2_osc_message_print((const LV2Message*)buffer);

	fprintf(stderr, "RAW:\n");
	for (uint32_t i=0; i < buffer_size; ++i) {
		unsigned char byte = ((unsigned char*)buffer)[i];
		if (byte >= 32 && byte <= 126)
			fprintf(stderr, "%c  ", ((unsigned char*)buffer)[i]);
		else
			fprintf(stderr, "%2X ", ((unsigned char*)buffer)[i]);
	}
	
	fprintf(stderr, "\n");
	*/

	NodeController* nc = (NodeController*)controller;

	SharedPtr<PortModel> port = nc->node->ports()[port_index];

	nc->engine->set_port_value_immediate(port->path(),
			port->type().uri(), buffer_size, buffer);
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


const string&
PluginModel::icon_path() const
{
	if (_icon_path == "" && _type == LV2)
		_icon_path = get_lv2_icon_path(_slv2_plugin);
	
	return _icon_path;
}


string
PluginModel::get_lv2_icon_path(SLV2Plugin plugin)
{
	string result;
	SLV2Values paths = slv2_plugin_get_value(plugin, SLV2_URI,
			"http://ll-plugins.nongnu.org/lv2/namespace#svgIcon");
	
	if (slv2_values_size(paths) > 0) {
		SLV2Value value = slv2_values_get_at(paths, 0);
		if (slv2_value_is_uri(value))
			result = slv2_uri_to_path(slv2_value_as_string(value));
		slv2_values_free(paths);
	}
	
	return result;
}

#endif

} // namespace Client
} // namespace Ingen
