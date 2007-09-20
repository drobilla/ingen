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

using std::cerr; using std::endl;

namespace Ingen {
namespace GUI {


NodeMenu::NodeMenu(SharedPtr<NodeModel> node)
	: _enable_signal(false)
	, _node(node)
	, _controls_menuitem(NULL)
{
	App& app = App::instance();

	Gtk::Menu_Helpers::MenuElem controls_elem = Gtk::Menu_Helpers::MenuElem("Controls",
		sigc::bind(
			sigc::mem_fun(app.window_factory(), &WindowFactory::present_controls),
			node));
	_controls_menuitem = controls_elem.get_child();
	items().push_back(controls_elem);
	
	items().push_back(Gtk::Menu_Helpers::SeparatorElem());

	items().push_back(Gtk::Menu_Helpers::MenuElem("Rename...",
		sigc::bind(
			sigc::mem_fun(app.window_factory(), &WindowFactory::present_rename),
			node)));
	
	/*items().push_back(Gtk::Menu_Helpers::MenuElem("Clone",
		sigc::bind(
			sigc::mem_fun(app.engine(), &EngineInterface::clone),
			node)));*/
	
	items().push_back(Gtk::Menu_Helpers::MenuElem("Disconnect All",
			sigc::mem_fun(this, &NodeMenu::on_menu_disconnect_all)));

	items().push_back(Gtk::Menu_Helpers::MenuElem("Destroy",
			sigc::mem_fun(this, &NodeMenu::on_menu_destroy)));
	
	//m_controls_menuitem->property_sensitive() = false;
	
	cerr << "FIXME: MIDI learn menu\n";
	/*
	if (_node->plugin() && _node->plugin()->type() == PluginModel::Internal
			&& _node->plugin()->plug_label() == "midi_control_in") {
		items().push_back(Gtk::Menu_Helpers::MenuElem("Learn",
			sigc::mem_fun(this, &NodeMenu::on_menu_learn)));
	}
	*/
	
	items().push_back(Gtk::Menu_Helpers::SeparatorElem());
	
	items().push_back(Gtk::Menu_Helpers::MenuElem("Properties",
		sigc::bind(
			sigc::mem_fun(app.window_factory(), &WindowFactory::present_properties),
			node)));
	
	Gtk::Menu_Helpers::CheckMenuElem poly_elem = Gtk::Menu_Helpers::CheckMenuElem(
			"Polyphonic", sigc::mem_fun(this, &NodeMenu::on_menu_polyphonic));
	items().push_back(poly_elem);
	_polyphonic_menuitem = static_cast<Gtk::RadioMenuItem*>(&items().back());
	_polyphonic_menuitem->set_active(node->polyphonic());

	node->signal_polyphonic.connect(sigc::mem_fun(this, &NodeMenu::polyphonic_changed));

	_enable_signal = true;

	//model->new_port_sig.connect(sigc::mem_fun(this, &NodeMenu::add_port));
	//model->destroyed_sig.connect(sigc::mem_fun(this, &NodeMenu::destroy));
}


void
NodeMenu::on_menu_destroy()
{
	App::instance().engine()->destroy(_node->path());
}


void
NodeMenu::on_menu_polyphonic()
{
	if (_enable_signal)
		App::instance().engine()->set_polyphonic(_node->path(), _polyphonic_menuitem->get_active());
}


void
NodeMenu::polyphonic_changed(bool polyphonic)
{
	_enable_signal = false;
	_polyphonic_menuitem->set_active(polyphonic);
	_enable_signal = true;
}


void
NodeMenu::on_menu_clone()
{
	cerr << "[NodeMenu] FIXME: clone broken\n";
	/*
	assert(_node);
	//assert(_parent != NULL);
	//assert(_parent->model() != NULL);
	
	string clone_name = _node->name();
	int i = 2; // postfix number (ie oldname_2)
	
	// Check if name already has a number postfix
	if (clone_name.find_last_of("_") != string::npos) {
		string name_postfix = clone_name.substr(clone_name.find_last_of("_")+1);
		clone_name = clone_name.substr(0, clone_name.find_last_of("_"));
		if (sscanf(name_postfix.c_str(), "%d", &i))
			++i;
	}
	
	char clone_postfix[4];
	for ( ; i < 100; ++i) {
		snprintf(clone_postfix, 4, "_%d", i);
		if (_node->parent_patch()->get_node(clone_name + clone_postfix) == NULL)
			break;
	}
	
	clone_name = clone_name + clone_postfix;
	
	const string path = _node->parent_patch()->base() + clone_name;
	NodeModel* nm = new NodeModel(_node->plugin(), path);
	nm->polyphonic(_node->polyphonic());
	nm->x(_node->x() + 20);
	nm->y(_node->y() + 20);
	App::instance().engine()->create_node_from_model(nm);
	*/
}


void
NodeMenu::on_menu_learn()
{
	App::instance().engine()->midi_learn(_node->path());
}

void
NodeMenu::on_menu_disconnect_all()
{
	App::instance().engine()->disconnect_all(_node->path());
}


bool
NodeMenu::has_control_inputs()
{
	for (PortModelList::const_iterator i = _node->ports().begin();
			i != _node->ports().end(); ++i)
		if ((*i)->is_input() && (*i)->is_control())
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

