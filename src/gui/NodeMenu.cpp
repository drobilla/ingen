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
#include <gtkmm.h>
#include "interface/EngineInterface.hpp"
#include "client/NodeModel.hpp"
#include "App.hpp"
#include "NodeMenu.hpp"
#include "WindowFactory.hpp"

using namespace std;
using std::cerr; using std::endl;

namespace Ingen {
namespace GUI {


NodeMenu::NodeMenu(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
	: ObjectMenu(cobject, xml)
	, _controls_menuitem(NULL)
{
	Gtk::Menu* node_menu = NULL;
	xml->get_widget("node_menu", node_menu);
	xml->get_widget("node_controls_menuitem", _controls_menuitem);
	xml->get_widget("node_popup_gui_menuitem", _popup_gui_menuitem);
	xml->get_widget("node_embed_gui_menuitem", _embed_gui_menuitem);
	xml->get_widget("node_randomize_menuitem", _randomize_menuitem);

	node_menu->remove(*_controls_menuitem);
	node_menu->remove(*_popup_gui_menuitem);
	node_menu->remove(*_embed_gui_menuitem);
	node_menu->remove(*_randomize_menuitem);
	
	insert(*_randomize_menuitem, 0);
	items().push_front(Gtk::Menu_Helpers::SeparatorElem());
	insert(*_controls_menuitem, 0);
	insert(*_popup_gui_menuitem, 0);
	insert(*_embed_gui_menuitem, 0);
}


void
NodeMenu::init(SharedPtr<NodeModel> node)
{
	ObjectMenu::init(node);
			
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

	if (has_control_inputs())
		_randomize_menuitem->show();
	else
		_randomize_menuitem->hide();

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

