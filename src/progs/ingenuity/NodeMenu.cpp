/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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
#include "NodeMenu.h"
#include "NodeModel.h"
#include "App.h"
#include "ModelEngineInterface.h"
#include "WindowFactory.h"

using std::cerr; using std::endl;

namespace Ingenuity {


NodeMenu::NodeMenu(CountedPtr<NodeModel> node)
: _node(node)
, _controls_menuitem(NULL)
{
	App& app = App::instance();

	Gtk::Menu_Helpers::MenuElem controls_elem = Gtk::Menu_Helpers::MenuElem("Controls",
		sigc::bind(
			sigc::mem_fun(app.window_factory(), &WindowFactory::present_controls),
			node));
	_controls_menuitem = controls_elem.get_child();
	items().push_back(controls_elem);

	items().push_back(Gtk::Menu_Helpers::MenuElem("Properties",
		sigc::bind(
			sigc::mem_fun(app.window_factory(), &WindowFactory::present_properties),
			node)));
	
	items().push_back(Gtk::Menu_Helpers::SeparatorElem());

	/*items().push_back(Gtk::Menu_Helpers::MenuElem("Rename...",
		sigc::bind(
			sigc::mem_fun(app.window_factory(), &WindowFactory::present_rename),
			node)));*/
	/*items().push_back(Gtk::Menu_Helpers::MenuElem("Clone",
		sigc::bind(
			sigc::mem_fun(app.engine(), &EngineInterface::clone),
			node)));
		sigc::mem_fun(this, &NodeMenu::on_menu_clone)));*/
	
	items().push_back(Gtk::Menu_Helpers::MenuElem("Disconnect All",
			sigc::mem_fun(this, &NodeMenu::on_menu_disconnect_all)));

	items().push_back(Gtk::Menu_Helpers::MenuElem("Destroy",
			sigc::mem_fun(this, &NodeMenu::on_menu_destroy)));
	
	//m_controls_menuitem->property_sensitive() = false;
	
	if (_node->plugin() && _node->plugin()->type() == PluginModel::Internal
			&& _node->plugin()->plug_label() == "midi_control_in") {
		items().push_back(Gtk::Menu_Helpers::MenuElem("Learn",
			sigc::mem_fun(this, &NodeMenu::on_menu_learn)));
	}

	//model->new_port_sig.connect(sigc::mem_fun(this, &NodeMenu::add_port));
	//model->destroyed_sig.connect(sigc::mem_fun(this, &NodeMenu::destroy));
}

#if 0
NodeMenu::~NodeMenu()
{
	cerr << "~NodeMenu()\n";
}

void
NodeMenu::destroy()
{
	cerr << "FIXME: NODE DESTROYED\n";
	//CountedPtr<ObjectModel> model = m_model;
	//m_model.reset();
}
#endif

void
NodeMenu::set_path(const Path& new_path)
{
	cerr << "FIXME: rename\n";
	/*
	remove_from_store();
	
	// Rename ports
	for (list<PortModel*>::const_iterator i = _node->ports().begin();
			i != _node->ports().end(); ++i) {
		ObjectController* const pc = (*i)->controller();
		assert(pc != NULL);
		pc->set_path(m_model->path().base() + pc->model()->name());
	}

	// Handle bridge port, if this node represents one
	if (m_bridge_port != NULL)
		m_bridge_port->set_path(new_path);

	if (m_module != NULL)
		m_module->canvas()->rename_module(_node->path().name(), new_path.name());
	
	ObjectController::set_path(new_path);
	
	add_to_store();
	*/
}

#if 0
void
NodeMenu::destroy()
{
	PatchController* pc = ((PatchController*)m_model->parent()->controller());
	assert(pc != NULL);

	//remove_from_store();
	//pc->remove_node(m_model->path().name());
	cerr << "FIXME: remove node\n";

	if (m_bridge_port != NULL)
		m_bridge_port->destroy();
	m_bridge_port = NULL;

	//if (m_module != NULL)
	//	delete m_module;
}
#endif

#if 0
void
NodeMenu::add_port(CountedPtr<PortModel> pm)
{
	assert(pm->parent().get() == _node.get());
	assert(pm->parent() == _node);
	assert(_node->get_port(pm->path().name()) == pm);
	
	//cout << "[NodeMenu] Adding port " << pm->path() << endl;
	
	/*
	if (m_module != NULL) {
		// (formerly PortController)
		pc->create_port(m_module);
		m_module->resize();
		
		// Enable "Controls" menu item on module
		if (has_control_inputs())
			enable_controls_menuitem();
	}*/
}
#endif

void
NodeMenu::on_menu_destroy()
{
	App::instance().engine()->destroy(_node->path());
}


void
NodeMenu::on_menu_clone()
{
	cerr << "FIXME: clone broken\n";
	/*
	assert(_node);
	//assert(m_parent != NULL);
	//assert(m_parent->model() != NULL);
	
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
	
	//if (m_control_window != NULL)
	//	m_control_window->hide();
}



} // namespace Ingenuity

