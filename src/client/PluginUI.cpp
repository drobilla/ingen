/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

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

static void
lv2_ui_write(SuilController controller,
             uint32_t       port_index,
             uint32_t       buffer_size,
             uint32_t       format,
             const void*    buffer)
{
	PluginUI* const ui = (PluginUI*)controller;

	const BlockModel::Ports& ports = ui->block()->ports();
	if (port_index >= ports.size()) {
		ui->world()->log().error(
			Raul::fmt("%1% UI tried to write to invalid port %2%\n")
			% ui->block()->plugin()->uri().c_str() % port_index);
		return;
	}

	SPtr<const PortModel> port = ports[port_index];

	const URIs& uris = ui->world()->uris();

	// float (special case, always 0)
	if (format == 0) {
		assert(buffer_size == 4);
		if (*(const float*)buffer == port->value().get_float())
			return; // do nothing (handle stupid plugin UIs that feed back)

		ui->world()->interface()->set_property(
			port->uri(),
			uris.ingen_value,
			ui->world()->forge().make(*(const float*)buffer));

	} else if (format == uris.atom_eventTransfer.id) {
		const LV2_Atom*  atom = (const LV2_Atom*)buffer;
		Raul::Atom val  = ui->world()->forge().alloc(
			atom->size, atom->type, LV2_ATOM_BODY_CONST(atom));
		ui->world()->interface()->set_property(port->uri(),
		                                       uris.ingen_value,
		                                       val);

	} else {
		ui->world()->log().warn(
			Raul::fmt("Unknown value format %1% from LV2 UI\n")
			% format % ui->block()->plugin()->uri().c_str());
	}
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
	suil_instance_free(_instance);
	lilv_node_free(_ui_node);
}

SPtr<PluginUI>
PluginUI::create(Ingen::World*          world,
                 SPtr<const BlockModel> block,
                 const LilvPlugin*      plugin)
{
	if (!PluginUI::ui_host) {
		PluginUI::ui_host = suil_host_new(lv2_ui_write, NULL, NULL, NULL);
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

	SPtr<PluginUI> ret(new PluginUI(world, block, lilv_ui_get_uri(ui)));
	ret->_features = world->lv2_features().lv2_features(
		world, const_cast<BlockModel*>(block.get()));

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
		world->log().error("Failed to instantiate LV2 UI\n");
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
