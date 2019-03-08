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

#ifndef INGEN_GUI_OBJECTMENU_HPP
#define INGEN_GUI_OBJECTMENU_HPP

#include "ingen/client/ObjectModel.hpp"
#include "ingen/types.hpp"

#include <gtkmm/builder.h>
#include <gtkmm/checkmenuitem.h>
#include <gtkmm/menu.h>
#include <gtkmm/menuitem.h>

namespace ingen {
namespace gui {

class App;
class ObjectControlWindow;
class ObjectPropertiesWindow;
class GraphCanvas;

/** Menu for a Object.
 *
 * \ingroup GUI
 */
class ObjectMenu : public Gtk::Menu
{
public:
	ObjectMenu(BaseObjectType*                   cobject,
	           const Glib::RefPtr<Gtk::Builder>& xml);

	void init(App& app, SPtr<const client::ObjectModel> object);

	SPtr<const client::ObjectModel> object() const { return _object; }
	App*                            app()    const { return _app; }

protected:
	void         on_menu_learn();
	void         on_menu_unlearn();
	virtual void on_menu_disconnect() = 0;
	void         on_menu_polyphonic();
	void         on_menu_destroy();
	void         on_menu_properties();

	void property_changed(const URI& predicate, const Atom& value);

	App*                            _app;
	SPtr<const client::ObjectModel> _object;
	Gtk::MenuItem*                  _learn_menuitem;
	Gtk::MenuItem*                  _unlearn_menuitem;
	Gtk::CheckMenuItem*             _polyphonic_menuitem;
	Gtk::MenuItem*                  _disconnect_menuitem;
	Gtk::MenuItem*                  _rename_menuitem;
	Gtk::MenuItem*                  _destroy_menuitem;
	Gtk::MenuItem*                  _properties_menuitem;
	Gtk::SeparatorMenuItem*         _separator_menuitem;

	bool _enable_signal;
};

} // namespace gui
} // namespace ingen

#endif // INGEN_GUI_OBJECTMENU_HPP
