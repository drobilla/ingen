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

	node_menu->remove(*_controls_menuitem);
	items().push_front(Gtk::Menu_Helpers::SeparatorElem());
	insert(*_controls_menuitem, 0);
}


void
NodeMenu::init(SharedPtr<NodeModel> node)
{
	ObjectMenu::init(node);
			
	_controls_menuitem->signal_activate().connect(sigc::bind(
			sigc::mem_fun(App::instance().window_factory(), &WindowFactory::present_controls),
			node));

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
	App::instance().engine()->midi_learn(_object->path());
}


bool
NodeMenu::has_control_inputs()
{
	const NodeModel* const nm = (NodeModel*)_object.get();
	for (PortModelList::const_iterator i = nm->ports().begin(); i != nm->ports().end(); ++i)
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

