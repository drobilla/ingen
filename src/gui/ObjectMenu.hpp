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

#ifndef INGEN_GUI_OBJECTMENU_HPP
#define INGEN_GUI_OBJECTMENU_HPP

#include <string>
#include <gtkmm.h>
#include <libglademm/xml.h>
#include "raul/SharedPtr.hpp"
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
	void         on_menu_learn();
	virtual void on_menu_disconnect() = 0;
	void         on_menu_polyphonic();
	void         on_menu_destroy();
	void         on_menu_properties();

	void property_changed(const Raul::URI& predicate, const Raul::Atom& value);

	bool                   _enable_signal;
	SharedPtr<ObjectModel> _object;
	Gtk::MenuItem*         _learn_menuitem;
	Gtk::CheckMenuItem*    _polyphonic_menuitem;
	Gtk::MenuItem*         _disconnect_menuitem;
	Gtk::MenuItem*         _rename_menuitem;
	Gtk::MenuItem*         _destroy_menuitem;
	Gtk::MenuItem*         _properties_menuitem;
};


} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_OBJECTMENU_HPP
