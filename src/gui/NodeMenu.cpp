/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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
#include <gtkmm.h>
#include "interface/EngineInterface.hpp"
#include "client/NodeModel.hpp"
#include "client/PluginModel.hpp"
#include "App.hpp"
#include "NodeMenu.hpp"
#include "WindowFactory.hpp"

using namespace std;
using namespace Ingen::Client;

namespace Ingen {
namespace GUI {


NodeMenu::NodeMenu(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
	: ObjectMenu(cobject, xml)
	, _controls_menuitem(NULL)
	, _presets_menu(NULL)
{
	Gtk::Menu* node_menu = NULL;
	xml->get_widget("node_menu", node_menu);
	xml->get_widget("node_learn_menuitem", _learn_menuitem);
	xml->get_widget("node_controls_menuitem", _controls_menuitem);
	xml->get_widget("node_popup_gui_menuitem", _popup_gui_menuitem);
	xml->get_widget("node_embed_gui_menuitem", _embed_gui_menuitem);
	xml->get_widget("node_randomize_menuitem", _randomize_menuitem);

	node_menu->remove(*_learn_menuitem);
	node_menu->remove(*_controls_menuitem);
	node_menu->remove(*_popup_gui_menuitem);
	node_menu->remove(*_embed_gui_menuitem);
	node_menu->remove(*_randomize_menuitem);

	insert(*_randomize_menuitem, 0);
	items().push_front(Gtk::Menu_Helpers::SeparatorElem());
	insert(*_controls_menuitem, 0);
	insert(*_popup_gui_menuitem, 0);
	insert(*_embed_gui_menuitem, 0);
	insert(*_learn_menuitem, 0);
}


void
NodeMenu::init(SharedPtr<NodeModel> node)
{
	ObjectMenu::init(node);

	_learn_menuitem->signal_activate().connect(sigc::mem_fun(this,
			&NodeMenu::on_menu_learn));
	_controls_menuitem->signal_activate().connect(sigc::bind(
			sigc::mem_fun(App::instance().window_factory(), &WindowFactory::present_controls),
			node));
	_popup_gui_menuitem->signal_activate().connect(sigc::mem_fun(signal_popup_gui,
			&sigc::signal<void>::emit));
	_embed_gui_menuitem->signal_toggled().connect(sigc::mem_fun(this,
			&NodeMenu::on_menu_embed_gui));
	_randomize_menuitem->signal_activate().connect(sigc::mem_fun(this,
			&NodeMenu::on_menu_randomize));

	const PluginModel* plugin = dynamic_cast<const PluginModel*>(node->plugin());
	if (plugin && plugin->type() == PluginModel::LV2 && plugin->has_ui()) {
		_popup_gui_menuitem->show();
		_embed_gui_menuitem->show();
	} else {
		_popup_gui_menuitem->hide();
		_embed_gui_menuitem->hide();
	}

#ifdef HAVE_SLV2
	if (plugin && plugin->type() == PluginModel::LV2) {
		SLV2Results presets = slv2_plugin_query_sparql(plugin->slv2_plugin(),
				"PREFIX pset: <http://lv2plug.in/ns/dev/presets#>\n"
				"PREFIX dc:   <http://dublincore.org/documents/dcmi-namespace/>\n"
				"SELECT ?p ?name WHERE { <> pset:hasPreset ?p . ?p dc:title ?name }\n");
		if (!slv2_results_finished(presets)) {
			items().push_front(Gtk::Menu_Helpers::SeparatorElem());
			items().push_front(Gtk::Menu_Helpers::ImageMenuElem("_Presets",
					*(manage(new Gtk::Image(Gtk::Stock::INDEX, Gtk::ICON_SIZE_MENU)))));
			Gtk::MenuItem* presets_menu_item = &(items().front());
			_presets_menu = Gtk::manage(new Gtk::Menu());
			presets_menu_item->set_submenu(*_presets_menu);
			for (; !slv2_results_finished(presets); slv2_results_next(presets)) {
				SLV2Value uri  = slv2_results_get_binding_value(presets, 0);
				SLV2Value name = slv2_results_get_binding_value(presets, 1);
				Gtk::MenuItem* item = Gtk::manage(new Gtk::MenuItem(slv2_value_as_string(name)));
				_presets_menu->items().push_back(*item);
				item->show();
				item->signal_activate().connect(
						sigc::bind(sigc::mem_fun(this, &NodeMenu::on_preset_activated),
								string(slv2_value_as_string(uri))), false);
			}
		}
		slv2_results_free(presets);
	}
#endif

	if (has_control_inputs())
		_randomize_menuitem->show();
	else
		_randomize_menuitem->hide();

	if (plugin && (plugin->uri().str() == "http://drobilla.net/ns/ingen-internals#Controller"
			|| plugin->uri().str() == "http://drobilla.net/ns/ingen-internals#Trigger"))
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
	App::instance().engine()->bundle_begin();

	const NodeModel* const nm = (NodeModel*)_object.get();
	for (NodeModel::Ports::const_iterator i = nm->ports().begin(); i != nm->ports().end(); ++i) {
		if ((*i)->is_input() && (*i)->type().is_control()) {
			float min = 0.0f, max = 1.0f;
			nm->port_value_range(*i, min, max);
			const float val = ((rand() / (float)RAND_MAX) * (max - min) + min);
			App::instance().engine()->set_port_value((*i)->path(), val);
		}
	}

	App::instance().engine()->bundle_end();
}


void
NodeMenu::on_menu_learn()
{
	App::instance().engine()->midi_learn(_object->path());
}


void
NodeMenu::on_menu_disconnect()
{
	App::instance().engine()->disconnect_all(_object->parent()->path(), _object->path());
}


void
NodeMenu::on_preset_activated(const std::string uri)
{
#ifdef HAVE_SLV2
	const NodeModel* const   node   = (NodeModel*)_object.get();
	const PluginModel* const plugin = dynamic_cast<const PluginModel*>(node->plugin());
	const string query = string(
			"PREFIX pset: <http://lv2plug.in/ns/dev/presets#>\n"
			"PREFIX dc:   <http://dublincore.org/documents/dcmi-namespace/>\n"
			"SELECT ?sym ?val WHERE { <") + uri + "> lv2:port ?port . "
				" ?port lv2:symbol ?sym ; pset:value ?val . }";
	SLV2Results values = slv2_plugin_query_sparql(plugin->slv2_plugin(), query.c_str());
	App::instance().engine()->bundle_begin();
	for (; !slv2_results_finished(values); slv2_results_next(values)) {
		SLV2Value sym  = slv2_results_get_binding_value(values, 0);
		SLV2Value val = slv2_results_get_binding_value(values, 1);
		App::instance().engine()->set_port_value(
				node->path().base() + slv2_value_as_string(sym),
				slv2_value_as_float(val));
	}
	App::instance().engine()->bundle_end();
	slv2_results_free(values);
#endif
}


bool
NodeMenu::has_control_inputs()
{
	const NodeModel* const nm = (NodeModel*)_object.get();
	for (NodeModel::Ports::const_iterator i = nm->ports().begin(); i != nm->ports().end(); ++i)
		if ((*i)->is_input() && (*i)->type().is_control())
			return true;

	return false;
}


void
NodeMenu::enable_controls_menuitem()
{
	_controls_menuitem->property_sensitive() = true;
}


void
NodeMenu::disable_controls_menuitem()
{
	_controls_menuitem->property_sensitive() = false;
}


} // namespace GUI
} // namespace Ingen

