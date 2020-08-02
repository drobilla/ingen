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

#include "NodeMenu.hpp"

#include "App.hpp"
#include "WidgetFactory.hpp"
#include "WindowFactory.hpp"

#include "ingen/Interface.hpp"
#include "ingen/Log.hpp"
#include "ingen/URIMap.hpp"
#include "ingen/client/BlockModel.hpp"
#include "ingen/client/PluginModel.hpp"
#include "ingen/client/PortModel.hpp"
#include "lv2/presets/presets.h"

#include <glib.h>
#include <gtkmm/entry.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/image.h>
#include <gtkmm/stock.h>

#include <cstdint>
#include <string>
#include <utility>

namespace ingen {

using namespace client;

namespace gui {

NodeMenu::NodeMenu(BaseObjectType*                   cobject,
                   const Glib::RefPtr<Gtk::Builder>& xml)
	: ObjectMenu(cobject, xml)
	, _presets_menu(nullptr)
{
	xml->get_widget("node_popup_gui_menuitem", _popup_gui_menuitem);
	xml->get_widget("node_embed_gui_menuitem", _embed_gui_menuitem);
	xml->get_widget("node_enabled_menuitem", _enabled_menuitem);
	xml->get_widget("node_randomize_menuitem", _randomize_menuitem);
}

void
NodeMenu::init(App& app, SPtr<const client::BlockModel> block)
{
	ObjectMenu::init(app, block);

	_learn_menuitem->signal_activate().connect(
		sigc::mem_fun(this, &NodeMenu::on_menu_learn));
	_popup_gui_menuitem->signal_activate().connect(
		sigc::mem_fun(signal_popup_gui, &sigc::signal<void>::emit));
	_embed_gui_menuitem->signal_toggled().connect(
		sigc::mem_fun(this, &NodeMenu::on_menu_embed_gui));
	_enabled_menuitem->signal_toggled().connect(
		sigc::mem_fun(this, &NodeMenu::on_menu_enabled));
	_randomize_menuitem->signal_activate().connect(
		sigc::mem_fun(this, &NodeMenu::on_menu_randomize));

	SPtr<PluginModel> plugin = block->plugin_model();
	if (plugin) {
		// Get the plugin to receive related presets
		_preset_connection = plugin->signal_preset().connect(
			sigc::mem_fun(this, &NodeMenu::add_preset));

		if (!plugin->fetched()) {
			_app->interface()->get(plugin->uri());
			plugin->set_fetched(true);
		}
	}

	if (plugin && plugin->has_ui()) {
		_popup_gui_menuitem->show();
		_embed_gui_menuitem->show();
		const Atom& ui_embedded = block->get_property(
			_app->uris().ingen_uiEmbedded);
		_embed_gui_menuitem->set_active(
			ui_embedded.is_valid() && ui_embedded.get<int32_t>());
	} else {
		_popup_gui_menuitem->hide();
		_embed_gui_menuitem->hide();
	}

	const Atom& enabled = block->get_property(_app->uris().ingen_enabled);
	_enabled_menuitem->set_active(!enabled.is_valid() || enabled.get<int32_t>());

	if (plugin && _app->uris().lv2_Plugin == plugin->type()) {
		_presets_menu = Gtk::manage(new Gtk::Menu());
		_presets_menu->items().push_back(
			Gtk::Menu_Helpers::MenuElem(
				"_Save Preset...",
				sigc::mem_fun(this, &NodeMenu::on_save_preset_activated)));
		_presets_menu->items().push_back(Gtk::Menu_Helpers::SeparatorElem());

		for (const auto& p : plugin->presets()) {
			add_preset(p.first, p.second);
		}

		items().push_front(
			Gtk::Menu_Helpers::ImageMenuElem(
				"_Presets",
				*(manage(new Gtk::Image(Gtk::Stock::INDEX, Gtk::ICON_SIZE_MENU)))));

		Gtk::MenuItem* presets_menu_item = &(items().front());
		presets_menu_item->set_submenu(*_presets_menu);
	}

	if (has_control_inputs()) {
		_randomize_menuitem->show();
	} else {
		_randomize_menuitem->hide();
	}

	if (plugin && (plugin->uri() == "http://drobilla.net/ns/ingen-internals#Controller"
	               || plugin->uri() == "http://drobilla.net/ns/ingen-internals#Trigger")) {
		_learn_menuitem->show();
	} else {
		_learn_menuitem->hide();
	}

	if (!_popup_gui_menuitem->is_visible() &&
	    !_embed_gui_menuitem->is_visible() &&
	    !_randomize_menuitem->is_visible()) {
		_separator_menuitem->hide();
	}

	_enable_signal = true;
}

void
NodeMenu::add_preset(const URI& uri, const std::string& label)
{
	if (_presets_menu) {
		_presets_menu->items().push_back(
			Gtk::Menu_Helpers::MenuElem(
				label,
				sigc::bind(sigc::mem_fun(this, &NodeMenu::on_preset_activated),
				           uri)));
	}
}

void
NodeMenu::on_menu_embed_gui()
{
	signal_embed_gui.emit(_embed_gui_menuitem->get_active());
}

void
NodeMenu::on_menu_enabled()
{
	_app->set_property(_object->uri(),
	                   _app->uris().ingen_enabled,
	                   _app->forge().make(bool(_enabled_menuitem->get_active())));
}

void
NodeMenu::on_menu_randomize()
{
	_app->interface()->bundle_begin();

	const SPtr<const BlockModel> bm = block();
	for (const auto& p : bm->ports()) {
		if (p->is_input() && _app->can_control(p.get())) {
			float min = 0.0f;
			float max = 1.0f;
			bm->port_value_range(p, min, max, _app->sample_rate());

			const float r = static_cast<float>(g_random_double_range(0.0, 1.0));
			const float val = r * (max - min) + min;
			_app->set_property(p->uri(),
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
NodeMenu::on_save_preset_activated()
{
	Gtk::FileChooserDialog dialog("Save Preset", Gtk::FILE_CHOOSER_ACTION_SAVE);
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);
	dialog.set_default_response(Gtk::RESPONSE_OK);
	dialog.set_current_folder(Glib::build_filename(Glib::get_home_dir(), ".lv2"));

	Gtk::HBox*  extra = Gtk::manage(new Gtk::HBox());
	Gtk::Label* label = Gtk::manage(new Gtk::Label("URI (Optional): "));
	Gtk::Entry* entry = Gtk::manage(new Gtk::Entry());
	extra->pack_start(*label, false, true, 4);
	extra->pack_start(*entry, true, true, 4);
	extra->show_all();
	dialog.set_extra_widget(*Gtk::manage(extra));

	if (dialog.run() == Gtk::RESPONSE_OK) {
		const std::string user_uri  = dialog.get_uri();
		const std::string user_path = Glib::filename_from_uri(user_uri);
		const std::string dirname   = Glib::path_get_dirname(user_path);
		const std::string basename  = Glib::path_get_basename(user_path);
		const std::string sym       = Raul::Symbol::symbolify(basename);
		const std::string plugname  = block()->plugin_model()->human_name();
		const std::string prefix    = Raul::Symbol::symbolify(plugname);
		const std::string bundle    = prefix + "_" + sym + ".preset.lv2/";
		const std::string file      = sym + ".ttl";
		const std::string real_path = Glib::build_filename(dirname, bundle, file);
		const std::string real_uri  = Glib::filename_to_uri(real_path);

		Properties props{
			{ _app->uris().rdf_type,
			  _app->uris().pset_Preset },
			{ _app->uris().rdfs_label,
			  _app->forge().alloc(basename) },
			{ _app->uris().lv2_prototype,
			  _app->forge().make_urid(block()->uri()) }};
		_app->interface()->put(URI(real_uri), props);
	}
}

void
NodeMenu::on_preset_activated(const std::string& uri)
{
	_app->set_property(block()->uri(),
	                   _app->uris().pset_preset,
	                   _app->forge().make_urid(URI(uri)));
}

bool
NodeMenu::has_control_inputs()
{
	for (const auto& p : block()->ports()) {
		if (p->is_input() && p->is_numeric()) {
			return true;
		}
	}

	return false;
}

} // namespace gui
} // namespace ingen
