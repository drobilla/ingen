/*
  This file is part of Ingen.
  Copyright 2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_GUI_URI_ENTRY_HPP
#define INGEN_GUI_URI_ENTRY_HPP

#include "ingen/URI.hpp"
#include "lilv/lilv.h"

#include <gdk/gdk.h>
#include <glibmm/signalproxy.h>
#include <glibmm/ustring.h>
#include <gtkmm/box.h>
#include <gtkmm/entry.h>

#include <set>
#include <string>

namespace Gtk {
class Button;
class Menu;
} // namespace Gtk

namespace ingen::gui {

class App;

class URIEntry : public Gtk::HBox
{
public:
	/** Create a widget for entering URIs.
	 *
	 * If `types` is given, then a menu button will be shown which pops up a
	 *	enu for easily choosing known values with valid types.
	 */
	URIEntry(App* app, std::set<URI> types, const std::string& value);

	std::string              get_text()       { return _entry->get_text(); }
	Glib::SignalProxy0<void> signal_changed() { return _entry->signal_changed(); }

private:
	Gtk::Menu* build_value_menu();
	Gtk::Menu* build_subclass_menu(const LilvNode* klass);

	void add_leaf_menu_item(Gtk::Menu*         menu,
	                        const LilvNode*    node,
	                        const std::string& label);

	void add_class_menu_item(Gtk::Menu*         menu,
	                         const LilvNode*    klass,
	                         const std::string& label);

	void uri_chosen(const std::string& uri);
	bool menu_button_event(GdkEvent* ev);

	App*                _app;
	const std::set<URI> _types;
	Gtk::Button*        _menu_button;
	Gtk::Entry*         _entry;
};

} // namespace ingen::gui

#endif // INGEN_GUI_URI_ENTRY_HPP
