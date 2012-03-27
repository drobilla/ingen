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

#ifndef INGEN_GUI_GLADEFACTORY_HPP
#define INGEN_GUI_GLADEFACTORY_HPP

#include <string>

#include <gtkmm.h>

namespace Ingen {
namespace GUI {

/** Loads widgets from an XML description.
 * Purely static.
 *
 * \ingroup GUI
 */
class WidgetFactory {
public:
	static Glib::RefPtr<Gtk::Builder>
	create(const std::string& toplevel_widget="");

	template<typename T>
	static void get_widget(const Glib::ustring& name, T*& widget) {
		Glib::RefPtr<Gtk::Builder> xml = create(name);
		xml->get_widget(name, widget);
	}

	template<typename T>
	static void get_widget_derived(const Glib::ustring& name, T*& widget) {
		Glib::RefPtr<Gtk::Builder> xml = create(name);
		xml->get_widget_derived(name, widget);
	}

private:
	static void find_ui_file();
	static Glib::ustring ui_filename;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_GLADEFACTORY_HPP
