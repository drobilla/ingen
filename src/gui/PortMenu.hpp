/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

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

#include <gtkmm/builder.h>
#include <gtkmm/menu.h>
#include <gtkmm/menushell.h>

#include "ingen/client/PortModel.hpp"
#include "ingen/types.hpp"

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

	void init(App&                          app,
	          SPtr<const Client::PortModel> port,
	          bool                          internal_graph_port = false);

private:
	void on_menu_disconnect();
	void on_menu_set_min();
	void on_menu_set_max();
	void on_menu_reset_range();
	void on_menu_expose();

	Gtk::Menu*     _port_menu;
	Gtk::MenuItem* _set_min_menuitem;
	Gtk::MenuItem* _set_max_menuitem;
	Gtk::MenuItem* _reset_range_menuitem;
	Gtk::MenuItem* _expose_menuitem;

	/// True iff this is a (flipped) port on a GraphPortModule in its graph
	bool _internal_graph_port;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_PORTMENU_HPP
