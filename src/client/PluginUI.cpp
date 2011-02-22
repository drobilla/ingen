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

#include "raul/log.hpp"

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/event/event.h"
#include "lv2/lv2plug.in/ns/ext/event/event-helpers.h"

#include "slv2/ui.h"

#include "shared/LV2Atom.hpp"
#include "shared/LV2URIMap.hpp"

#include "NodeModel.hpp"
#include "PluginUI.hpp"
#include "PortModel.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Client {

static void
lv2_ui_write(LV2UI_Controller controller,
             uint32_t         port_index,
             uint32_t         buffer_size,
             uint32_t         format,
             const void*      buffer)
{
	PluginUI* const ui = (PluginUI*)controller;

	const NodeModel::Ports& ports = ui->node()->ports();
	if (port_index >= ports.size()) {
		error << "UI for " << ui->node()->plugin()->uri()
		      << " tried to write to non-existent port " << port_index << endl;
		return;
	}

	SharedPtr<PortModel> port = ports[port_index];

	const Shared::LV2URIMap& uris = *ui->world()->uris().get();

	// float (special case, always 0)
	if (format == 0) {
		assert(buffer_size == 4);
		if (*(float*)buffer == port->value().get_float())
			return; // do nothing (handle stupid plugin UIs that feed back)

		ui->world()->engine()->set_property(port->path(),
		                                    uris.ingen_value,
		                                    Atom(*(float*)buffer));

	} else if (format == uris.ui_Events.id) {
		LV2_Event_Buffer*  buf = (LV2_Event_Buffer*)buffer;
		LV2_Event_Iterator iter;
		uint8_t*           data;
		lv2_event_begin(&iter, buf);
		while (lv2_event_is_valid(&iter)) {
			LV2_Event* const ev = lv2_event_get(&iter, &data);
			if (ev->type == uris.midi_MidiEvent.id) {
				// FIXME: bundle multiple events by writing an entire buffer here
				ui->world()->engine()->set_property(
					port->path(),
					uris.ingen_value,
					Atom("http://lv2plug.in/ns/ext/midi#MidiEvent", ev->size, data));
			} else {
				warn << "Unable to serialise UI event type " << ev->type
				     << ", event lost" << endl;
			}

			lv2_event_increment(&iter);
		}

	} else if (format == uris.atom_AtomTransfer.id) {
		LV2_Atom* buf = (LV2_Atom*)buffer;
		Raul::Atom val;
		Shared::LV2Atom::to_atom(uris, buf, val);
		ui->world()->engine()->set_property(port->path(), uris.ingen_value, val);

	} else {
		warn << "Unknown value format " << format
		     << " from LV2 UI " << ui->node()->plugin()->uri() << endl;
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
	suil_instance_free(_instance);
}


SharedPtr<PluginUI>
PluginUI::create(Ingen::Shared::World* world,
                 SharedPtr<NodeModel>  node,
                 SLV2Plugin            plugin)
{
	SharedPtr<PluginUI> ret(new PluginUI(world, node));
	ret->_features = world->lv2_features()->lv2_features(world, node.get());

	// Build Suil UI set for this plugin
	const char* const plugin_uri = slv2_value_as_uri(slv2_plugin_get_uri(plugin));
	SLV2UIs           slv2_uis   = slv2_plugin_get_uis(plugin);
	SuilUIs           suil_uis   = suil_uis_new(plugin_uri);
	for (unsigned i = 0; i < slv2_uis_size(slv2_uis); ++i) {
		SLV2UI     ui      = slv2_uis_get_at(slv2_uis, i);
		SLV2Values classes = slv2_ui_get_classes(ui);
		for (unsigned j = 0; j < slv2_values_size(classes); ++j) {
			SLV2Value type = slv2_values_get_at(classes, j);
			if (suil_ui_type_supported("http://lv2plug.in/ns/extensions/ui#GtkUI",
			                           slv2_value_as_uri(type))) {
				suil_uis_add(
					suil_uis,
					slv2_value_as_uri(slv2_ui_get_uri(ui)),
					slv2_value_as_uri(type),
					slv2_uri_to_path(slv2_value_as_uri(slv2_ui_get_bundle_uri(ui))),
					slv2_uri_to_path(slv2_value_as_uri(slv2_ui_get_binary_uri(ui))));
				break;
			} else {
				warn << "Unsupported LV2 UI type " << slv2_value_as_uri(type) << endl;
			}
		}
	}

	// Attempt to instantiate a UI
	SuilInstance instance = suil_instance_new(
		suil_uis,
		"http://lv2plug.in/ns/extensions/ui#GtkUI",
		NULL,
		lv2_ui_write,
		ret.get(),
		ret->_features->array());

	if (instance) {
		ret->_instance = instance;
	} else {
		error << "Failed to instantiate LV2 UI" << endl;
		ret.reset();
	}

	return ret;
}

LV2UI_Widget
PluginUI::get_widget()
{
	return (LV2UI_Widget*)suil_instance_get_widget(_instance);
}

void
PluginUI::port_event(uint32_t    port_index,
                     uint32_t    buffer_size,
                     uint32_t    format,
                     const void* buffer)
{
	suil_instance_port_event(_instance, port_index, buffer_size, format, buffer);
}

} // namespace Client
} // namespace Ingen
