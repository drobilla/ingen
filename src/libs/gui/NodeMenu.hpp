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

#ifndef NODEMENU_H
#define NODEMENU_H

#include <string>
#include <gtkmm.h>
#include <raul/Path.hpp>
#include <raul/SharedPtr.hpp>
#include "client/NodeModel.hpp"
#include "ObjectMenu.hpp"

using Ingen::Client::NodeModel;

namespace Ingen {
namespace GUI {

class Controller;
class NodeControlWindow;
class NodePropertiesWindow;
class PatchCanvas;

/** Controller for a Node.
 *
 * \ingroup GUI
 */
class NodeMenu : public ObjectMenu
{
public:
	NodeMenu(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml);
	
	virtual void program_add(int bank, int program, const std::string& name) {}
	virtual void program_remove(int bank, int program) {}

	void init(SharedPtr<NodeModel> node);

	bool has_control_inputs();
	
protected:
	
	virtual void enable_controls_menuitem();
	virtual void disable_controls_menuitem();
	void show_gui();

	void on_menu_clone();
	void on_menu_learn();

	Gtk::MenuItem* _controls_menuitem;
	Gtk::MenuItem* _gui_menuitem;
};


} // namespace GUI
} // namespace Ingen

#endif // NODEMENU_H
