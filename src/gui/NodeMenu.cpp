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

#include <string>

#include <gtkmm/image.h>
#include <gtkmm/stock.h>

#include "ingen/Interface.hpp"
#include "ingen/Log.hpp"
#include "ingen/client/BlockModel.hpp"
#include "ingen/client/PluginModel.hpp"
#include "lv2/lv2plug.in/ns/ext/presets/presets.h"

#include "App.hpp"
#include "NodeMenu.hpp"
#include "WidgetFactory.hpp"
#include "WindowFactory.hpp"

using namespace std;

namespace Ingen {

using namespace Client;

namespace GUI {

NodeMenu::NodeMenu(BaseObjectType*                   cobject,
                   const Glib::RefPtr<Gtk::Builder>& xml)
	: ObjectMenu(cobject, xml)
	, _presets_menu(NULL)
{
	xml->get_widget("node_popup_gui_menuitem", _popup_gui_menuitem);
	xml->get_widget("node_embed_gui_menuitem", _embed_gui_menuitem);
	xml->get_widget("node_randomize_menuitem", _randomize_menuitem);
}

void
NodeMenu::init(App& app, SPtr<const Client::BlockModel> block)
{
	ObjectMenu::init(app, block);

	_learn_menuitem->signal_activate().connect(sigc::mem_fun(this,
			&NodeMenu::on_menu_learn));
	_popup_gui_menuitem->signal_activate().connect(
		sigc::mem_fun(signal_popup_gui, &sigc::signal<void>::emit));
	_embed_gui_menuitem->signal_toggled().connect(
		sigc::mem_fun(this, &NodeMenu::on_menu_embed_gui));
	_randomize_menuitem->signal_activate().connect(
		sigc::mem_fun(this, &NodeMenu::on_menu_randomize));

	const PluginModel* plugin = dynamic_cast<const PluginModel*>(block->plugin());
	if (plugin && plugin->type() == PluginModel::LV2 && plugin->has_ui()) {
		_popup_gui_menuitem->show();
		_embed_gui_menuitem->show();
		const Raul::Atom& ui_embedded = block->get_property(
			_app->uris().ingen_uiEmbedded);
		_embed_gui_menuitem->set_active(
			ui_embedded.is_valid() && ui_embedded.get<int32_t>());
	} else {
		_popup_gui_menuitem->hide();
		_embed_gui_menuitem->hide();
	}

	if (plugin && plugin->type() == PluginModel::LV2) {

		LilvNode* pset_Preset = lilv_new_uri(plugin->lilv_world(),
		                                     LV2_PRESETS__Preset);
		LilvNode* rdfs_label = lilv_new_uri(plugin->lilv_world(),
		                                    LILV_NS_RDFS "label");
		LilvNodes* presets = lilv_plugin_get_related(plugin->lilv_plugin(),
		                                             pset_Preset);
		if (presets) {
			_presets_menu = Gtk::manage(new Gtk::Menu());

			LILV_FOREACH(nodes, i, presets) {
				const LilvNode* preset = lilv_nodes_get(presets, i);
				lilv_world_load_resource(plugin->lilv_world(), preset);
				LilvNodes* labels = lilv_world_find_nodes(
					plugin->lilv_world(), preset, rdfs_label, NULL);
				if (labels) {
					const LilvNode* label = lilv_nodes_get_first(labels);
					_presets_menu->items().push_back(
						Gtk::Menu_Helpers::MenuElem(
							lilv_node_as_string(label),
							sigc::bind(
								sigc::mem_fun(this, &NodeMenu::on_preset_activated),
								string(lilv_node_as_string(preset)))));

					// I have no idea why this is necessary, signal_activated doesn't work
					// in this menu (and only this menu)
					Gtk::MenuItem* item = &(_presets_menu->items().back());
					item->signal_button_release_event().connect(
						sigc::bind<0>(sigc::mem_fun(this, &NodeMenu::on_preset_clicked),
						              string(lilv_node_as_string(preset))));

					lilv_nodes_free(labels);
				} else {
					app.log().error(
						Raul::fmt("Preset <%1> has no rdfs:label\n")
						% lilv_node_as_string(lilv_nodes_get(presets, i)));
				}
			}

			items().push_front(Gtk::Menu_Helpers::ImageMenuElem("_Presets",
			                                                    *(manage(new Gtk::Image(Gtk::Stock::INDEX, Gtk::ICON_SIZE_MENU)))));
			Gtk::MenuItem* presets_menu_item = &(items().front());
			presets_menu_item->set_submenu(*_presets_menu);
			lilv_nodes_free(presets);
		}
		lilv_node_free(pset_Preset);
		lilv_node_free(rdfs_label);
	}

	if (has_control_inputs())
		_randomize_menuitem->show();
	else
		_randomize_menuitem->hide();

	if (plugin && (plugin->uri() == "http://drobilla.net/ns/ingen-internals#Controller"
			|| plugin->uri() == "http://drobilla.net/ns/ingen-internals#Trigger"))
		_learn_menuitem->show();
	else
		_learn_menuitem->hide();

	_enable_signal = true;
}

void
NodeMenu::on_menu_embed_gui()
{
	signal_embed_gui.emit(_embed_gui_menuitem->get_active());
}

void
NodeMenu::on_menu_randomize()
{
	_app->interface()->bundle_begin();

	const BlockModel* const bm = (const BlockModel*)_object.get();
	for (const auto& p : bm->ports()) {
		if (p->is_input() && _app->can_control(p.get())) {
			float min = 0.0f, max = 1.0f;
			bm->port_value_range(p, min, max, _app->sample_rate());
			const float val = g_random_double_range(0.0, 1.0) * (max - min) + min;
			_app->interface()->set_property(
				p->uri(),
				_app->uris().ingen_value,
				_app->forge().make(val));
		}
	}

	_app->interface()->bundle_end();
}

void
NodeMenu::on_menu_disconnect()
{
	_app->interface()->disconnect_all(_object->parent()->path(), _object->path());
}

void
NodeMenu::on_preset_activated(const std::string& uri)
{
	const BlockModel* const  block  = (const BlockModel*)_object.get();
	const PluginModel* const plugin = dynamic_cast<const PluginModel*>(block->plugin());

	LilvNode* port_pred = lilv_new_uri(plugin->lilv_world(),
	                                   LV2_CORE__port);
	LilvNode* symbol_pred = lilv_new_uri(plugin->lilv_world(),
	                                     LV2_CORE__symbol);
	LilvNode* value_pred = lilv_new_uri(plugin->lilv_world(),
	                                    LV2_PRESETS__value);
	LilvNode*  subject = lilv_new_uri(plugin->lilv_world(), uri.c_str());
	LilvNodes* ports   = lilv_world_find_nodes(
		plugin->lilv_world(),
		subject,
		port_pred,
		NULL);
	_app->interface()->bundle_begin();
	LILV_FOREACH(nodes, i, ports) {
		const LilvNode* uri = lilv_nodes_get(ports, i);
		LilvNodes* values = lilv_world_find_nodes(
			plugin->lilv_world(), uri, value_pred, NULL);
		LilvNodes* symbols = lilv_world_find_nodes(
			plugin->lilv_world(), uri, symbol_pred, NULL);
		if (values && symbols) {
			const LilvNode* val = lilv_nodes_get_first(values);
			const LilvNode* sym = lilv_nodes_get_first(symbols);
			_app->interface()->set_property(
				Node::path_to_uri(
					block->path().child(Raul::Symbol(lilv_node_as_string(sym)))),
				_app->uris().ingen_value,
				_app->forge().make(lilv_node_as_float(val)));
		}
	}
	_app->interface()->bundle_end();
	lilv_nodes_free(ports);
	lilv_node_free(value_pred);
	lilv_node_free(symbol_pred);
	lilv_node_free(port_pred);
}

bool
NodeMenu::on_preset_clicked(const std::string& uri, GdkEventButton* ev)
{
	on_preset_activated(uri);
	return false;
}

bool
NodeMenu::has_control_inputs()
{
	const BlockModel* const bm = (const BlockModel*)_object.get();
	for (const auto& p : bm->ports())
		if (p->is_input() && p->is_numeric())
			return true;

	return false;
}

} // namespace GUI
} // namespace Ingen
