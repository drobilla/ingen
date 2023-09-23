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

#include "ObjectMenu.hpp"

#include <memory>

namespace Glib {
template <class T> class RefPtr;
} // namespace Glib

namespace Gtk {
class Builder;
class Menu;
class MenuItem;
} // namespace Gtk

namespace ingen {

namespace client {
class PortModel;
} // namespace client

namespace gui {

class App;

/** Menu for a Port.
 *
 * \ingroup GUI
 */
class PortMenu : public ObjectMenu
{
public:
	PortMenu(BaseObjectType*                   cobject,
	         const Glib::RefPtr<Gtk::Builder>& xml);

	void init(App&                                            app,
	          const std::shared_ptr<const client::PortModel>& port,
	          bool internal_graph_port = false);

private:
	void on_menu_disconnect() override;
	void on_menu_set_min();
	void on_menu_set_max();
	void on_menu_reset_range();
	void on_menu_expose();

	Gtk::Menu*     _port_menu{nullptr};
	Gtk::MenuItem* _set_min_menuitem{nullptr};
	Gtk::MenuItem* _set_max_menuitem{nullptr};
	Gtk::MenuItem* _reset_range_menuitem{nullptr};
	Gtk::MenuItem* _expose_menuitem{nullptr};

	/// True iff this is a (flipped) port on a GraphPortModule in its graph
	bool _internal_graph_port{false};
};

} // namespace gui
} // namespace ingen

#endif // INGEN_GUI_PORTMENU_HPP
