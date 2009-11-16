/* This file is part of Ingen.
 * Copyright (C) 2009 Dave Robillard <http://drobilla.net>
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

#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <gtkmm.h>
#include <libglademm.h>

namespace Raul { class Path; }

namespace Ingen {

namespace Client { class ClientStore; }

namespace GUI {

class PatchWindow;
class PatchTreeView;


/** Ingen GUI Window
 * \ingroup GUI
 */
class Window : public Gtk::Window
{
public:
	Window()                        : Gtk::Window()        {}
	Window(BaseObjectType* cobject) : Gtk::Window(cobject) {}

	bool on_key_press_event(GdkEventKey* event) {
		if (Ingen::GUI::Window::key_press_handler(this, event))
			return true;
		else
			return Gtk::Window::on_key_press_event(event);
	}

	static bool key_press_handler(Gtk::Window* win, GdkEventKey* event);
};


/** Ingen GUI Dialog
 * \ingroup GUI
 */
class Dialog : public Gtk::Dialog
{
public:
	Dialog()                        : Gtk::Dialog()        {}
	Dialog(BaseObjectType* cobject) : Gtk::Dialog(cobject) {}

	bool on_key_press_event(GdkEventKey* event) {
		if (Ingen::GUI::Window::key_press_handler(this, event))
			return true;
		else
			return Gtk::Dialog::on_key_press_event(event);
	}
};


} // namespace GUI
} // namespace Ingen

#endif // WINDOW_HPP
