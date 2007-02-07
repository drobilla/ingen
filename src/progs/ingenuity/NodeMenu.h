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

#ifndef NODEMENU_H
#define NODEMENU_H

#include <string>
#include <gtkmm.h>
#include "raul/Path.h"
#include "raul/SharedPtr.h"
#include "NodeModel.h"
using Ingen::Client::NodeModel;

using std::string;

namespace Ingenuity {

class Controller;
class NodeControlWindow;
class NodePropertiesWindow;
class PatchCanvas;

/** Controller for a Node.
 *
 * \ingroup Ingenuity
 */
class NodeMenu : public Gtk::Menu
{
public:
	NodeMenu(SharedPtr<NodeModel> node);

	void set_path(const Path& new_path);
	
	virtual void program_add(int bank, int program, const string& name) {}
	virtual void program_remove(int bank, int program) {}

	bool has_control_inputs();
	
	//virtual void show_menu(GdkEventButton* event)
	//{ _menu.popup(event->button, event->time); }

protected:
	
	virtual void enable_controls_menuitem();
	virtual void disable_controls_menuitem();

	//virtual void add_port(SharedPtr<PortModel> pm);

	void on_menu_destroy();
	void on_menu_clone();
	void on_menu_learn();
	void on_menu_disconnect_all();

	//Gtk::Menu                   _menu;
	SharedPtr<NodeModel>       _node;
	Glib::RefPtr<Gtk::MenuItem> _controls_menuitem;
};


} // namespace Ingenuity

#endif // NODEMENU_H
