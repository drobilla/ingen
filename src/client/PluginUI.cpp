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

#include "ingen/shared/LV2Atom.hpp"
#include "shared/LV2URIMap.hpp"

#include "ingen/client/NodeModel.hpp"
#include "ingen/client/PluginUI.hpp"
#include "ingen/client/PortModel.hpp"

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

	SharedPtr<const PortModel> port = ports[port_index];

	const Shared::URIs&      uris    = *ui->world()->uris().get();
	const Shared::LV2URIMap& uri_map = *ui->world()->lv2_uri_map().get();

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
				uri_map.global_to_event(uris.midi_MidiEvent.id);
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

PluginUI::PluginUI(Ingen::Shared::World*      world,
                   SharedPtr<const NodeModel> node)
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
PluginUI::create(Ingen::Shared::World*      world,
                 SharedPtr<const NodeModel> node,
                 const LilvPlugin*          plugin)
{
	if (!PluginUI::ui_host) {
		PluginUI::ui_host = suil_host_new(lv2_ui_write, NULL, NULL, NULL);
	}

	static const char* gtk_ui_uri = "http://lv2plug.in/ns/extensions/ui#GtkUI";

	LilvNode* gtk_ui = lilv_new_uri(world->lilv_world(), gtk_ui_uri);

	LilvUIs*        uis     = lilv_plugin_get_uis(plugin);
	const LilvUI*   ui      = NULL;
	const LilvNode* ui_type = NULL;
	LILV_FOREACH(uis, u, uis) {
		const LilvUI* this_ui = lilv_uis_get(uis, u);
		if (lilv_ui_is_supported(this_ui,
		                         suil_ui_supported,
		                         gtk_ui,
		                         &ui_type)) {
			// TODO: Multiple UI support
			ui = this_ui;
			break;
		}
	}

	if (!ui) {
		lilv_node_free(gtk_ui);
		return SharedPtr<PluginUI>();
	}

	SharedPtr<PluginUI> ret(new PluginUI(world, node));
	ret->_features = world->lv2_features()->lv2_features(
		world, const_cast<NodeModel*>(node.get()));

	SuilInstance* instance = suil_instance_new(
		PluginUI::ui_host,
		ret.get(),
		lilv_node_as_uri(gtk_ui),
		lilv_node_as_uri(lilv_plugin_get_uri(plugin)),
		lilv_node_as_uri(lilv_ui_get_uri(ui)),
		lilv_node_as_uri(ui_type),
		lilv_uri_to_path(lilv_node_as_uri(lilv_ui_get_bundle_uri(ui))),
		lilv_uri_to_path(lilv_node_as_uri(lilv_ui_get_binary_uri(ui))),
		ret->_features->array());

	lilv_node_free(gtk_ui);

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
