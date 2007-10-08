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

#ifndef OBJECTMENU_H
#define OBJECTMENU_H

#include <string>
#include <gtkmm.h>
#include <libglademm/xml.h>
#include <raul/Path.hpp>
#include <raul/SharedPtr.hpp>
#include "client/ObjectModel.hpp"
using Ingen::Client::ObjectModel;

namespace Ingen {
namespace GUI {

class ObjectControlWindow;
class ObjectPropertiesWindow;
class PatchCanvas;

/** Menu for a Object.
 *
 * \ingroup GUI
 */
class ObjectMenu : public Gtk::Menu
{
public:
	ObjectMenu(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml);
	
	void init(SharedPtr<ObjectModel> object);

protected:

	void on_menu_polyphonic();
	void on_menu_disconnect();
	void on_menu_destroy();
	void on_menu_properties();

	void polyphonic_changed(bool polyphonic);

	bool                   _enable_signal;
	SharedPtr<ObjectModel> _object;
	Gtk::CheckMenuItem*    _polyphonic_menuitem;
	Gtk::MenuItem*         _disconnect_menuitem;
	Gtk::MenuItem*         _rename_menuitem;
	Gtk::MenuItem*         _destroy_menuitem;
	Gtk::MenuItem*         _properties_menuitem;
};


} // namespace GUI
} // namespace Ingen

#endif // OBJECTMENU_H
