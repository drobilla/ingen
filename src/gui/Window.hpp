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

#ifndef INGEN_GUI_WINDOW_HPP
#define INGEN_GUI_WINDOW_HPP

#include <gtkmm/dialog.h>
#include <gtkmm/window.h>

namespace Ingen {

namespace GUI {

class App;

/** Ingen GUI Window
 * \ingroup GUI
 */
class Window : public Gtk::Window
{
public:
	Window()                                 : Gtk::Window(), _app(NULL)        {}
	explicit Window(BaseObjectType* cobject) : Gtk::Window(cobject), _app(NULL) {}

	virtual void init_window(App& app) { _app = &app; }

	bool on_key_press_event(GdkEventKey* event) {
		if (event->keyval == GDK_w && event->state & GDK_CONTROL_MASK) {
			hide();
			return true;
		}
		return Gtk::Window::on_key_press_event(event);
	}

	static bool key_press_handler(Gtk::Window* win, GdkEventKey* event);

	App* _app;
};

/** Ingen GUI Dialog
 * \ingroup GUI
 */
class Dialog : public Gtk::Dialog
{
public:
	Dialog()                                 : Gtk::Dialog(), _app(NULL)        {}
	explicit Dialog(BaseObjectType* cobject) : Gtk::Dialog(cobject), _app(NULL) {}

	virtual void init_dialog(App& app) { _app = &app; }

	bool on_key_press_event(GdkEventKey* event) {
		if (event->keyval == GDK_w && event->state & GDK_CONTROL_MASK) {
			hide();
			return true;
		}
		return Gtk::Window::on_key_press_event(event);
	}

	App* _app;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_WINDOW_HPP
