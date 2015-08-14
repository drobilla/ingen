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

#include "ingen/Interface.hpp"
#include "ingen/Log.hpp"
#include "ingen/URIs.hpp"
#include "ingen/client/BlockModel.hpp"
#include "ingen/client/PluginUI.hpp"
#include "ingen/client/PortModel.hpp"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"

using namespace std;

namespace Ingen {
namespace Client {

SuilHost* PluginUI::ui_host = NULL;

static SPtr<const PortModel>
get_port(PluginUI* ui, uint32_t port_index)
{
	if (port_index >= ui->block()->ports().size()) {
		ui->world()->log().error(
			fmt("%1% UI tried to access invalid port %2%\n")
			% ui->block()->plugin()->uri().c_str() % port_index);
		return SPtr<const PortModel>();
	}
	return ui->block()->ports()[port_index];
}

static void
lv2_ui_write(SuilController controller,
             uint32_t       port_index,
             uint32_t       buffer_size,
             uint32_t       format,
             const void*    buffer)
{
	PluginUI* const       ui   = (PluginUI*)controller;
	const URIs&           uris = ui->world()->uris();
	SPtr<const PortModel> port = get_port(ui, port_index);
	if (!port) {
		return;
	}

	// float (special case, always 0)
	if (format == 0) {
		if (buffer_size != 4) {
			ui->world()->log().error(
				fmt("%1% UI wrote corrupt float with bad size\n")
				% ui->block()->plugin()->uri().c_str());
			return;
		}
		const float value = *(const float*)buffer;
		if (port->value().type() == uris.atom_Float &&
		    value == port->value().get<float>()) {
			return;  // Ignore feedback
		}

		ui->signal_property_changed()(
			port->uri(),
			uris.ingen_value,
			ui->world()->forge().make(value));

	} else if (format == uris.atom_eventTransfer.urid.get<LV2_URID>()) {
		const LV2_Atom* atom = (const LV2_Atom*)buffer;
		Atom            val  = ui->world()->forge().alloc(
			atom->size, atom->type, LV2_ATOM_BODY_CONST(atom));
		ui->signal_property_changed()(port->uri(),
		                              uris.ingen_activity,
		                              val);

	} else {
		ui->world()->log().warn(
			fmt("Unknown value format %1% from LV2 UI\n")
			% format % ui->block()->plugin()->uri().c_str());
	}
}

static uint32_t
lv2_ui_port_index(SuilController controller, const char* port_symbol)
{
	PluginUI* const ui = (PluginUI*)controller;

	const BlockModel::Ports& ports = ui->block()->ports();
	for (uint32_t i = 0; i < ports.size(); ++i) {
		if (ports[i]->symbol() == port_symbol) {
			return i;
		}
	}
	return LV2UI_INVALID_PORT_INDEX;
}

static uint32_t
lv2_ui_subscribe(SuilController            controller,
                 uint32_t                  port_index,
                 uint32_t                  protocol,
                 const LV2_Feature* const* features)
{
	PluginUI* const       ui   = (PluginUI*)controller;
	SPtr<const PortModel> port = get_port(ui, port_index);
	if (!port) {
		return 1;
	}

	ui->signal_property_changed()(
		ui->block()->ports()[port_index]->uri(),
		ui->world()->uris().ingen_broadcast,
		ui->world()->forge().make(true));

	return 0;
}

static uint32_t
lv2_ui_unsubscribe(SuilController            controller,
                   uint32_t                  port_index,
                   uint32_t                  protocol,
                   const LV2_Feature* const* features)
{
	PluginUI* const       ui   = (PluginUI*)controller;
	SPtr<const PortModel> port = get_port(ui, port_index);
	if (!port) {
		return 1;
	}

	ui->signal_property_changed()(
		ui->block()->ports()[port_index]->uri(),
		ui->world()->uris().ingen_broadcast,
		ui->world()->forge().make(false));

	return 0;
}

PluginUI::PluginUI(Ingen::World*          world,
                   SPtr<const BlockModel> block,
                   const LilvNode*        ui_node)
	: _world(world)
	, _block(block)
	, _instance(NULL)
	, _ui_node(lilv_node_duplicate(ui_node))
{
}

PluginUI::~PluginUI()
{
	for (uint32_t i : _subscribed_ports) {
		lv2_ui_unsubscribe(this, i, 0, NULL);
	}
	suil_instance_free(_instance);
	lilv_node_free(_ui_node);
}

SPtr<PluginUI>
PluginUI::create(Ingen::World*          world,
                 SPtr<const BlockModel> block,
                 const LilvPlugin*      plugin)
{
	if (!PluginUI::ui_host) {
		PluginUI::ui_host = suil_host_new(lv2_ui_write,
		                                  lv2_ui_port_index,
		                                  lv2_ui_subscribe,
		                                  lv2_ui_unsubscribe);
	}

	static const char* gtk_ui_uri = LV2_UI__GtkUI;

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
		return SPtr<PluginUI>();
	}

	// Create the PluginUI, but don't instantiate yet
	SPtr<PluginUI> ret(new PluginUI(world, block, lilv_ui_get_uri(ui)));
	ret->_features = world->lv2_features().lv2_features(
		world, const_cast<BlockModel*>(block.get()));

	/* Subscribe (enable broadcast) for any requested port notifications.  This
	   must be done before instantiation so responses to any events sent by the
	   UI's init() will be sent back to this client. */
	LilvWorld* lworld              = world->lilv_world();
	LilvNode*  ui_portNotification = lilv_new_uri(lworld, LV2_UI__portNotification);
	LilvNode*  ui_plugin           = lilv_new_uri(lworld, LV2_UI__plugin);
	LilvNode*  lv2_symbol          = lilv_new_uri(lworld, LV2_CORE__symbol);
	LilvNodes* notes               = lilv_world_find_nodes(
		lworld, lilv_ui_get_uri(ui), ui_portNotification, NULL);
	LILV_FOREACH(nodes, n, notes) {
		const LilvNode* note = lilv_nodes_get(notes, n);
		const LilvNode* sym  = lilv_world_get(lworld, note, lv2_symbol, NULL);
		const LilvNode* plug = lilv_world_get(lworld, note, ui_plugin, NULL);
		if (plug && !lilv_node_is_uri(plug)) {
			world->log().error(fmt("%1% UI has non-URI ui:plugin\n")
			                   % block->plugin()->uri().c_str());
			continue;
		}
		if (plug && strcmp(lilv_node_as_uri(plug), block->plugin()->uri().c_str())) {
			continue;  // Notification is for another plugin
		}
		if (sym) {
			uint32_t index = lv2_ui_port_index(ret.get(), lilv_node_as_string(sym));
			if (index != LV2UI_INVALID_PORT_INDEX) {
				lv2_ui_subscribe(ret.get(), index, 0, NULL);
				ret->_subscribed_ports.insert(index);
			}
		}
	}
	lilv_nodes_free(notes);
	lilv_node_free(lv2_symbol);
	lilv_node_free(ui_plugin);
	lilv_node_free(ui_portNotification);

	const char* bundle_uri  = lilv_node_as_uri(lilv_ui_get_bundle_uri(ui));
	const char* binary_uri  = lilv_node_as_uri(lilv_ui_get_binary_uri(ui));
	char*       bundle_path = lilv_file_uri_parse(bundle_uri, NULL);
	char*       binary_path = lilv_file_uri_parse(binary_uri, NULL);

	// Instantiate the actual plugin UI via Suil
	SuilInstance* instance = suil_instance_new(
		PluginUI::ui_host,
		ret.get(),
		lilv_node_as_uri(gtk_ui),
		lilv_node_as_uri(lilv_plugin_get_uri(plugin)),
		lilv_node_as_uri(lilv_ui_get_uri(ui)),
		lilv_node_as_uri(ui_type),
		bundle_path,
		binary_path,
		ret->_features->array());

	lilv_free(binary_path);
	lilv_free(bundle_path);
	lilv_node_free(gtk_ui);

	if (!instance) {
		world->log().error("Failed to instantiate LV2 UI\n");
		// Cancel any subscriptions
		for (uint32_t i : ret->_subscribed_ports) {
			lv2_ui_unsubscribe(ret.get(), i, 0, NULL);
		}
		return SPtr<PluginUI>();
	}

	ret->_instance = instance;

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

bool
PluginUI::is_resizable() const
{
	LilvWorld*      w   = _world->lilv_world();
	const LilvNode* s   = _ui_node;
	LilvNode*       p   = lilv_new_uri(w, LV2_CORE__optionalFeature);
	LilvNode*       fs  = lilv_new_uri(w, LV2_UI__fixedSize);
	LilvNode*       nrs = lilv_new_uri(w, LV2_UI__noUserResize);

	LilvNodes* fs_matches = lilv_world_find_nodes(w, s, p, fs);
	LilvNodes* nrs_matches = lilv_world_find_nodes(w, s, p, nrs);

	lilv_nodes_free(nrs_matches);
	lilv_nodes_free(fs_matches);
	lilv_node_free(nrs);
	lilv_node_free(fs);
	lilv_node_free(p);

	return !fs_matches && !nrs_matches;
}

} // namespace Client
} // namespace Ingen
