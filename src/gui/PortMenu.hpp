/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_GUI_PORTMENU_HPP
#define INGEN_GUI_PORTMENU_HPP

#include <string>

#include <gtkmm/builder.h>
#include <gtkmm/menu.h>
#include <gtkmm/menushell.h>

#include "ingen/client/PortModel.hpp"
#include "raul/SharedPtr.hpp"

#include "ObjectMenu.hpp"

namespace Ingen {
namespace GUI {

/** Menu for a Port.
 *
 * \ingroup GUI
 */
class PortMenu : public ObjectMenu
{
public:
	PortMenu(BaseObjectType*                   cobject,
	         const Glib::RefPtr<Gtk::Builder>& xml);

	void init(App&                               app,
	          SharedPtr<const Client::PortModel> port,
	          bool                               patch_port = false);

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
