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

#include "BreadCrumbBox.h"
#include "BreadCrumb.h"
namespace Ingenuity {


BreadCrumbBox::BreadCrumbBox()
: Gtk::HBox()
, _active_path("/")
, _full_path("/")
, _enable_signal(true)
{
}


/** Destroys current breadcrumbs and rebuilds from scratch.
 * 
 * (Needs to be called when a patch is cleared to eliminate children crumbs)
 */
void
BreadCrumbBox::build(Path path)
{
	bool old_enable_signal = _enable_signal;
	_enable_signal = false;

	// Moving to a parent path, just switch the active button
	if (path.length() < _full_path.length() && _full_path.substr(0, path.length()) == path) {
		
		for (std::list<BreadCrumb*>::iterator i = _breadcrumbs.begin(); i != _breadcrumbs.end(); ++i)
			(*i)->set_active( ((*i)->path() == path) );
	
		_active_path = path;
		_enable_signal = old_enable_signal;
		return;
	}

	// Otherwise rebuild from scratch
	_full_path = path;
	_active_path = path;

	// Empty existing breadcrumbs
	for (std::list<BreadCrumb*>::iterator i = _breadcrumbs.begin(); i != _breadcrumbs.end(); ++i)
		remove(**i);
	_breadcrumbs.clear();

	// Add root
	BreadCrumb* but = manage(new BreadCrumb("/"));
	but->signal_toggled().connect(sigc::bind(sigc::mem_fun(
					this, &BreadCrumbBox::breadcrumb_clicked), but));
	pack_start(*but, false, false, 1);
	_breadcrumbs.push_front(but);
	but->set_active(but->path() == _active_path);

	// Add the others
	while (path != "/") {
		BreadCrumb* but = manage(new BreadCrumb(path));
		but->signal_toggled().connect(sigc::bind(sigc::mem_fun(
					this, &BreadCrumbBox::breadcrumb_clicked), but));
		pack_start(*but, false, false, 1);
		_breadcrumbs.push_front(but);
		but->set_active(but->path() == _active_path);
		path = path.parent();
	}

	show_all_children();

	_enable_signal = old_enable_signal;
}


void
BreadCrumbBox::breadcrumb_clicked(BreadCrumb* crumb)
{
	if (_enable_signal) {
		_enable_signal = false;

		if (!crumb->get_active()) {
			assert(_active_path == crumb->path());
			// Tried to turn off the current active button, bad user!
			crumb->set_active(true);
		} else {
			signal_patch_selected.emit(crumb->path());
			if (crumb->path() != _active_path)
				crumb->set_active(false);
		}
		_enable_signal = true;
	}
}

} // namespace Ingenuity

