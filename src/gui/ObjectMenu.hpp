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

#include <ingen/URI.hpp>

#include <gtkmm/menu.h>

#include <memory>

namespace Glib {
template <class T> class RefPtr;
} // namespace Glib

namespace Gtk {
class Builder;
class CheckMenuItem;
class MenuItem;
class SeparatorMenuItem;
} // namespace Gtk

namespace ingen {

class Atom;

namespace client {
class ObjectModel;
} // namespace client

namespace gui {

class App;

/** Menu for a Object.
 *
 * \ingroup GUI
 */
class ObjectMenu : public Gtk::Menu
{
public:
	ObjectMenu(BaseObjectType*                   cobject,
	           const Glib::RefPtr<Gtk::Builder>& xml);

	void
	init(App& app, const std::shared_ptr<const client::ObjectModel>& object);

	std::shared_ptr<const client::ObjectModel> object() const
	{
		return _object;
	}

	App* app() const { return _app; }

protected:
	void         on_menu_learn();
	void         on_menu_unlearn();
	virtual void on_menu_disconnect() = 0;
	void         on_menu_polyphonic();
	void         on_menu_destroy();
	void         on_menu_properties();

	void property_changed(const URI& predicate, const Atom& value);

	App*                                       _app{nullptr};
	std::shared_ptr<const client::ObjectModel> _object;
	Gtk::MenuItem*                             _learn_menuitem{nullptr};
	Gtk::MenuItem*                             _unlearn_menuitem{nullptr};
	Gtk::CheckMenuItem*                        _polyphonic_menuitem{nullptr};
	Gtk::MenuItem*                             _disconnect_menuitem{nullptr};
	Gtk::MenuItem*                             _rename_menuitem{nullptr};
	Gtk::MenuItem*                             _destroy_menuitem{nullptr};
	Gtk::MenuItem*                             _properties_menuitem{nullptr};
	Gtk::SeparatorMenuItem*                    _separator_menuitem{nullptr};

	bool _enable_signal{false};
};

} // namespace gui
} // namespace ingen

#endif // INGEN_GUI_OBJECTMENU_HPP
