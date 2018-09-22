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
#include "lv2/atom/atom.h"
#include "lv2/ui/ui.h"

namespace Ingen {
namespace Client {

SuilHost* PluginUI::ui_host = nullptr;

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
			ui->world()->forge().make(value),
			Resource::Graph::DEFAULT);

	} else if (format == uris.atom_eventTransfer.urid.get<LV2_URID>()) {
		const LV2_Atom* atom = (const LV2_Atom*)buffer;
		Atom            val  = ui->world()->forge().alloc(
			atom->size, atom->type, LV2_ATOM_BODY_CONST(atom));
		ui->signal_property_changed()(port->uri(),
		                              uris.ingen_activity,
		                              val,
		                              Resource::Graph::DEFAULT);
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
		ui->world()->forge().make(true),
		Resource::Graph::DEFAULT);

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
		ui->world()->forge().make(false),
		Resource::Graph::DEFAULT);

	return 0;
}

PluginUI::PluginUI(Ingen::World*          world,
                   SPtr<const BlockModel> block,
                   LilvUIs*               uis,
                   const LilvUI*          ui,
                   const LilvNode*        ui_type)
	: _world(world)
	, _block(std::move(block))
	, _instance(nullptr)
	, _uis(uis)
	, _ui(ui)
	, _ui_node(lilv_node_duplicate(lilv_ui_get_uri(ui)))
	, _ui_type(lilv_node_duplicate(ui_type))
{
}

PluginUI::~PluginUI()
{
	for (uint32_t i : _subscribed_ports) {
		lv2_ui_unsubscribe(this, i, 0, nullptr);
	}
	suil_instance_free(_instance);
	lilv_node_free(_ui_node);
	lilv_node_free(_ui_type);
	lilv_uis_free(_uis);
	lilv_world_unload_resource(_world->lilv_world(), lilv_ui_get_uri(_ui));
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
	const LilvUI*   ui      = nullptr;
	const LilvNode* ui_type = nullptr;
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
	SPtr<PluginUI> ret(new PluginUI(world, block, uis, ui, ui_type));
	ret->_features = world->lv2_features().lv2_features(
		world, const_cast<BlockModel*>(block.get()));

	return ret;
}

bool
PluginUI::instantiate()
{
	const URIs&       uris       = _world->uris();
	const std::string plugin_uri = _block->plugin()->uri();
	LilvWorld*        lworld     = _world->lilv_world();

	// Load seeAlso files to access data like portNotification descriptions
	lilv_world_load_resource(lworld, lilv_ui_get_uri(_ui));

	/* Subscribe (enable broadcast) for any requested port notifications.  This
	   must be done before instantiation so responses to any events sent by the
	   UI's init() will be sent back to this client. */
	LilvNode*  ui_portNotification = lilv_new_uri(lworld, LV2_UI__portNotification);
	LilvNode*  ui_plugin           = lilv_new_uri(lworld, LV2_UI__plugin);
	LilvNodes* notes               = lilv_world_find_nodes(
		lworld, lilv_ui_get_uri(_ui), ui_portNotification, nullptr);
	LILV_FOREACH(nodes, n, notes) {
		const LilvNode* note = lilv_nodes_get(notes, n);
		const LilvNode* sym  = lilv_world_get(lworld, note, uris.lv2_symbol, nullptr);
		const LilvNode* plug = lilv_world_get(lworld, note, ui_plugin, nullptr);
		if (!plug) {
			_world->log().error(fmt("%1% UI %2% notification missing plugin\n")
			                    % plugin_uri % lilv_node_as_string(_ui_node));
		} else if (!sym) {
			_world->log().error(fmt("%1% UI %2% notification missing symbol\n")
			                    % plugin_uri % lilv_node_as_string(_ui_node));
		} else if (!lilv_node_is_uri(plug)) {
			_world->log().error(fmt("%1% UI %2% notification has non-URI plugin\n")
			                    % plugin_uri % lilv_node_as_string(_ui_node));
		} else if (!strcmp(lilv_node_as_uri(plug), plugin_uri.c_str())) {
			// Notification is valid and for this plugin
			uint32_t index = lv2_ui_port_index(this, lilv_node_as_string(sym));
			if (index != LV2UI_INVALID_PORT_INDEX) {
				lv2_ui_subscribe(this, index, 0, nullptr);
				_subscribed_ports.insert(index);
			}
		}
	}
	lilv_nodes_free(notes);
	lilv_node_free(ui_plugin);
	lilv_node_free(ui_portNotification);

	const char* bundle_uri  = lilv_node_as_uri(lilv_ui_get_bundle_uri(_ui));
	const char* binary_uri  = lilv_node_as_uri(lilv_ui_get_binary_uri(_ui));
	char*       bundle_path = lilv_file_uri_parse(bundle_uri, nullptr);
	char*       binary_path = lilv_file_uri_parse(binary_uri, nullptr);

	// Instantiate the actual plugin UI via Suil
	_instance = suil_instance_new(
		PluginUI::ui_host,
		this,
		LV2_UI__GtkUI,
		plugin_uri.c_str(),
		lilv_node_as_uri(lilv_ui_get_uri(_ui)),
		lilv_node_as_uri(_ui_type),
		bundle_path,
		binary_path,
		_features->array());

	lilv_free(binary_path);
	lilv_free(bundle_path);

	if (!_instance) {
		_world->log().error("Failed to instantiate LV2 UI\n");
		// Cancel any subscriptions
		for (uint32_t i : _subscribed_ports) {
			lv2_ui_unsubscribe(this, i, 0, nullptr);
		}
		return false;
	}

	return true;
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
	if (_instance) {
		suil_instance_port_event(
			_instance, port_index, buffer_size, format, buffer);
	} else {
		_world->log().warn("LV2 UI port event with no instance\n");
	}
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
