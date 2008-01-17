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

#include <iostream>
#include "PluginUI.hpp"
#include "NodeModel.hpp"
#include "PortModel.hpp"

using namespace std;
using Ingen::Shared::EngineInterface;

namespace Ingen {
namespace Client {

static void
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

	PluginUI* ui = (PluginUI*)controller;

	SharedPtr<PortModel> port = ui->node()->ports()[port_index];

	ui->engine()->set_port_value_immediate(port->path(),
			port->type().uri(), buffer_size, buffer);
}

	
PluginUI::PluginUI(SharedPtr<EngineInterface> engine,
                   SharedPtr<NodeModel>       node)
	: _engine(engine)
	, _node(node)
	, _instance(NULL)
{
}


PluginUI::~PluginUI()
{
	slv2_ui_instance_free(_instance);
}
	

SharedPtr<PluginUI>
PluginUI::create(SharedPtr<EngineInterface> engine,
                 SharedPtr<NodeModel>       node,
                 SLV2Plugin                 plugin)
{
	SharedPtr<PluginUI> ret;

	static const char* gtk_gui_uri = "http://ll-plugins.nongnu.org/lv2/ext/ui#GtkUI";

	SLV2UIs uis = slv2_plugin_get_uis(plugin);
	SLV2UI  ui  = NULL;

	if (slv2_values_size(uis) > 0) {
		for (unsigned i=0; i < slv2_uis_size(uis); ++i) {
			SLV2UI this_ui = slv2_uis_get_at(uis, i);
			if (slv2_ui_is_type(this_ui, gtk_gui_uri)) {
				ui = this_ui;
				break;
			}
		}
	}

	if (ui) {
		cout << "Found GTK Plugin UI: " << slv2_ui_get_uri(ui) << endl;
		ret = SharedPtr<PluginUI>(new PluginUI(engine, node));
		SLV2UIInstance inst = slv2_ui_instantiate(
				plugin, ui, lv2_ui_write, ret.get(), NULL, NULL);

		if (inst) {
			ret->set_instance(inst);
		} else {
			cerr << "ERROR: Failed to instantiate Plugin UI" << endl;
			ret = SharedPtr<PluginUI>();
		}
	}

	return ret;
}


} // namespace Client
} // namespace Ingen
