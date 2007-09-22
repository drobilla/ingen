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


void
lv2_ui_write(LV2UI_Controller controller,
              uint32_t        port,
              uint32_t        buffer_size,
              const void*     buffer)
{
	cerr << "********* LV2 UI WRITE port " << port << ", size "
		<< buffer_size << endl;
	for (uint32_t i=0; i < buffer_size; ++i) {
		fprintf(stderr, "( %X )", *((uint8_t*)buffer + i));
	}
	fprintf(stderr, "\n");
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
PluginModel::ui()
{
	if (_type != LV2)
		return NULL;

	Glib::Mutex::Lock(_rdf_world->mutex());
	
	SLV2UIInstance ret = NULL;

	SLV2Values ui = slv2_plugin_get_uis(_slv2_plugin);
	if (slv2_values_size(ui) > 0) {
		printf("\tUIs:\n");
		for (unsigned i=0; i < slv2_values_size(ui); ++i) {
			printf("\t\t%s\n", slv2_value_as_uri(slv2_values_get_at(ui, i)));

			SLV2Value binary = slv2_plugin_get_ui_library_uri(_slv2_plugin, slv2_values_get_at(ui, i));

			printf("\t\t\tType: %s\n", slv2_ui_type_get_uri(slv2_value_as_ui_type(
							slv2_values_get_at(ui, i))));

			if (binary)
				printf("\t\t\tBinary: %s\n", slv2_value_as_uri(binary));

			slv2_value_free(binary);
		}
	
		if (slv2_values_size(ui) > 1)
			printf("WARNING:  Multiple UIs found, using the first...");
		
		ret = slv2_plugin_ui_instantiate(_slv2_plugin,
				slv2_values_get_at(ui, 0),
				lv2_ui_write, lv2_ui_command,
				lv2_ui_program_change, lv2_ui_program_save,
				NULL, NULL);
	}

	slv2_values_free(ui);

	return ret;
}

#endif

} // namespace Client
} // namespace Ingen
