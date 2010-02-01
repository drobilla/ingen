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

#ifndef INGEN_GUI_PORTMENU_HPP
#define INGEN_GUI_PORTMENU_HPP

#include <string>
#include <gtkmm.h>
#include "raul/SharedPtr.hpp"
#include "client/PortModel.hpp"
#include "ObjectMenu.hpp"

using Ingen::Client::PortModel;

namespace Ingen {
namespace GUI {


/** Menu for a Port.
 *
 * \ingroup GUI
 */
class PortMenu : public ObjectMenu
{
public:
	PortMenu(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml);

	void init(SharedPtr<PortModel> port, bool patch_port = false);

private:
	void on_menu_disconnect();
	void on_menu_set_min();
	void on_menu_set_max();
	void on_menu_reset_range();

	bool _patch_port;

	Gtk::Menu*     _port_menu;
	Gtk::MenuItem* _set_min_menuitem;
	Gtk::MenuItem* _set_max_menuitem;
	Gtk::MenuItem* _reset_range_menuitem;
};


} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_PORTMENU_HPP
