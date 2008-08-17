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
#include "lv2ext/lv2_event_helpers.h"
#include "shared/LV2URIMap.hpp"
#include "PluginUI.hpp"
#include "NodeModel.hpp"
#include "PortModel.hpp"

using namespace std;
using Ingen::Shared::EngineInterface;
using Ingen::Shared::LV2URIMap;
using Ingen::Shared::LV2Features;

namespace Ingen {
namespace Client {

static void
lv2_ui_write(LV2UI_Controller controller,
             uint32_t         port_index,
             uint32_t         buffer_size,
             uint32_t         format,
             const void*      buffer)
{
	/*
	cerr << "lv2_ui_write (format " << format << "):" << endl;
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
	
	const LV2Features::Feature* f = ui->world()->lv2_features->feature(LV2_URI_MAP_URI);
	LV2URIMap* map = (LV2URIMap*)f->controller;
	assert(map);

	// float (special case, always 0)
	if (format == 0) {
		assert(buffer_size == 4);
		if (*(float*)buffer == port->value().get_float())
			return; // do nothing (handle stupid plugin UIs that feed back)
	
		ui->world()->engine->set_port_value_immediate(port->path(), Atom(*(float*)buffer));

	// FIXME: slow, need to cache ID
	} else if (format == map->uri_to_id(NULL, "http://lv2plug.in/ns/extensions/ui#Events")) {
		uint32_t midi_event_type = map->uri_to_id(NULL, "http://lv2plug.in/ns/ext/midi#MidiEvent");
		LV2_Event_Buffer* buf = (LV2_Event_Buffer*)buffer;
		LV2_Event_Iterator iter;
		uint8_t* data;
		lv2_event_begin(&iter, buf);
		while (lv2_event_is_valid(&iter)) {
			LV2_Event* const ev = lv2_event_get(&iter, &data);
			if (ev->type == midi_event_type) {
				// FIXME: bundle multiple events by writing an entire buffer here
				ui->world()->engine->set_port_value_immediate(port->path(),
					Atom("lv2_midi:MidiEvent", ev->size, data));
			} else {
				cerr << "WARNING: Unable to send event type " << ev->type << 
					" over OSC, ignoring event" << endl;
			}

			lv2_event_increment(&iter);
		}
	} else {
		cerr << "WARNING: Unknown value format " << format
			<< ", either plugin " << ui->node()->plugin()->uri() << " is broken"
			<< " or this is an Ingen bug" << endl;
	}
}

	
PluginUI::PluginUI(Ingen::Shared::World* world,
                   SharedPtr<NodeModel>  node)
	: _world(world)
	, _node(node)
	, _instance(NULL)
{
}


PluginUI::~PluginUI()
{
	Glib::Mutex::Lock lock(PluginModel::rdf_world()->mutex());
	slv2_ui_instance_free(_instance);
}


SharedPtr<PluginUI>
PluginUI::create(Ingen::Shared::World* world,
                 SharedPtr<NodeModel>  node,
                 SLV2Plugin            plugin)
{
	Glib::Mutex::Lock lock(PluginModel::rdf_world()->mutex());
	SharedPtr<PluginUI> ret;

	SLV2Value gtk_gui_uri = slv2_value_new_uri(world->slv2_world,
		"http://lv2plug.in/ns/extensions/ui#GtkUI");

	SLV2UIs uis = slv2_plugin_get_uis(plugin);
	SLV2UI  ui  = NULL;

	if (slv2_values_size(uis) > 0) {
		for (unsigned i=0; i < slv2_uis_size(uis); ++i) {
			SLV2UI this_ui = slv2_uis_get_at(uis, i);
			if (slv2_ui_is_a(this_ui, gtk_gui_uri)) {
				ui = this_ui;
				break;
			}
		}
	}

	if (ui) {
		cout << "Found GTK Plugin UI: " << slv2_ui_get_uri(ui) << endl;
		ret = SharedPtr<PluginUI>(new PluginUI(world, node));
		SLV2UIInstance inst = slv2_ui_instantiate(
				plugin, ui, lv2_ui_write, ret.get(), world->lv2_features->lv2_features());

		if (inst) {
			ret->set_instance(inst);
		} else {
			cerr << "ERROR: Failed to instantiate Plugin UI" << endl;
			ret = SharedPtr<PluginUI>();
		}
	}

	slv2_value_free(gtk_gui_uri);
	return ret;
}


} // namespace Client
} // namespace Ingen
