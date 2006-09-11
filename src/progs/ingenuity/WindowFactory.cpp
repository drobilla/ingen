/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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

#include "WindowFactory.h"
#include "PatchWindow.h"
#include "GladeFactory.h"
#include "App.h"

namespace Ingenuity {


/** Present a PatchWindow for a Patch.
 *
 * If @a preferred is not NULL, it will be set to display @a patch if the patch
 * does not already have a visible window, otherwise that window will be presented and
 * @a preferred left unmodified.
 */
void
WindowFactory::present(CountedPtr<PatchController> patch, PatchWindow* preferred)
{
	std::map<Path, PatchWindow*>::iterator w = _windows.find(patch->model()->path());

	if (w != _windows.end()) {
		(*w).second->present();
	} else if (preferred) {
		w = _windows.find(preferred->patch_controller()->model()->path());
		assert((*w).second == preferred);

		preferred->set_patch(patch);
		_windows.erase(w);
		_windows[patch->model()->path()] = preferred;
		preferred->present();

	} else {
		PatchWindow* win = create_new(patch);
		win->present();
	}
}


PatchWindow*
WindowFactory::create_new(CountedPtr<PatchController> patch)
{
	// FIXME: can't just load "patch_win" and children because PatchWindow
	// loads things it really shouldn't.
	//Glib::RefPtr<Gnome::Glade::Xml> xml = GladeFactory::new_glade_reference("patch_win");
	Glib::RefPtr<Gnome::Glade::Xml> xml = GladeFactory::new_glade_reference();

	PatchWindow* win = NULL;
	xml->get_widget_derived("patch_win", win);
	assert(win);
	
	win->set_patch(patch);
	_windows[patch->model()->path()] = win;

	win->signal_delete_event().connect(sigc::bind<0>(
		sigc::mem_fun(this, &WindowFactory::remove), win));

	cerr << "Created window - count: " << _windows.size() << endl;

	return win;
}


bool
WindowFactory::remove(PatchWindow* win, GdkEventAny* ignored)
{
	if (_windows.size() <= 1) {
		Gtk::MessageDialog d(*win, "This is the last remaining open patch "
			"window.  Closing this window will exit Ingenuity (the engine will "
			"remain running).\n\nAre you sure you want to quit?",
			true, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_NONE, true);
			d.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
			d.add_button(Gtk::Stock::QUIT, Gtk::RESPONSE_CLOSE);
		int ret = d.run();
		if (ret == Gtk::RESPONSE_CLOSE)
			App::instance().quit();
		else
			return false;
	}

	std::map<Path, PatchWindow*>::iterator w
		= _windows.find(win->patch_controller()->model()->path());

	assert((*w).second == win);
	_windows.erase(w);

	delete win;
	
	cerr << "Removed window " << win << "- count: " << _windows.size() << endl;
	
	return true;
}

} // namespace Ingenuity
