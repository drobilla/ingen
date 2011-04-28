/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
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

#include "shared/LV2Atom.hpp"
#include "shared/LV2URIMap.hpp"

#include "NodeModel.hpp"
#include "PluginUI.hpp"
#include "PortModel.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Client {

SuilHost* PluginUI::ui_host = NULL;

static void
lv2_ui_write(SuilController controller,
             uint32_t       port_index,
             uint32_t       buffer_size,
             uint32_t       format,
             const void*    buffer)
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
			std::pair<bool, uint16_t> midi_id =
			  uris.global_to_event(uris.midi_MidiEvent.id);
			if (midi_id.first && ev->type == midi_id.second) {
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
	if (!PluginUI::ui_host) {
		PluginUI::ui_host = suil_host_new(lv2_ui_write, NULL, NULL, NULL);
	}

	static const char* gtk_ui_uri = "http://lv2plug.in/ns/extensions/ui#GtkUI";

	SLV2Value gtk_ui = slv2_value_new_uri(world->slv2_world(), gtk_ui_uri);

	SLV2UIs      uis      = slv2_plugin_get_uis(plugin);
	SLV2UI       ui       = NULL;
	SLV2Value    ui_type  = NULL;
	SLV2_FOREACH(u, uis) {
		SLV2UI this_ui = slv2_uis_get(uis, u);
		if (slv2_ui_is_supported(this_ui,
		                         suil_ui_supported,
		                         gtk_ui,
		                         &ui_type)) {
			// TODO: Multiple UI support
			ui = this_ui;
			break;
		}
	}

	if (!ui) {
		slv2_value_free(gtk_ui);
		return SharedPtr<PluginUI>();
	}

	SharedPtr<PluginUI> ret(new PluginUI(world, node));
	ret->_features = world->lv2_features()->lv2_features(world, node.get());

	SuilInstance* instance = suil_instance_new(
		PluginUI::ui_host,
		ret.get(),
		slv2_value_as_uri(gtk_ui),
		slv2_value_as_uri(slv2_plugin_get_uri(plugin)),
		slv2_value_as_uri(slv2_ui_get_uri(ui)),
		slv2_value_as_uri(ui_type),
		slv2_uri_to_path(slv2_value_as_uri(slv2_ui_get_bundle_uri(ui))),
		slv2_uri_to_path(slv2_value_as_uri(slv2_ui_get_binary_uri(ui))),
		ret->_features->array());

	slv2_value_free(gtk_ui);

	if (instance) {
		ret->_instance = instance;
	} else {
		error << "Failed to instantiate LV2 UI" << endl;
		ret.reset();
	}

	return ret;
}

SuilWidget
PluginUI::get_widget()
{
	return (SuilWidget*)suil_instance_get_widget(_instance);
}

void
PluginUI::port_event(uint32_t    port_index,
                     uint32_t    buffer_size,
                     uint32_t    format,
                     const void* buffer)
{
	suil_instance_port_event(
		_instance, port_index, buffer_size, format, buffer);
}

} // namespace Client
} // namespace Ingen
