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


CountedPtr<PatchView>
BreadCrumbBox::view(const Path& path)
{
	for (std::list<BreadCrumb*>::const_iterator i = _breadcrumbs.begin(); i != _breadcrumbs.end(); ++i)
		if ((*i)->path() == path)
			return (*i)->view();

	return CountedPtr<PatchView>();
}


/** Sets up the crumbs to display a @a path.
 *
 * If @a path is already part of the shown path, it will be selected and the 
 * children preserved.
 */
void
BreadCrumbBox::build(Path path, CountedPtr<PatchView> view)
{
	bool old_enable_signal = _enable_signal;
	_enable_signal = false;

	// Moving to a path we already contain, just switch the active button
	if (_breadcrumbs.size() > 0 && (path.is_parent_of(_full_path) || path == _full_path)) {
		
		for (std::list<BreadCrumb*>::iterator i = _breadcrumbs.begin(); i != _breadcrumbs.end(); ++i) {
			if ((*i)->path() == path) {
				(*i)->set_active(true);
				if (!(*i)->view())
					(*i)->set_view(view);

				// views are expensive, having two around for the same patch is a bug
				assert((*i)->view() == view);

			} else {
				(*i)->set_active(false);
			}
		}
	
		_active_path = path;
		_enable_signal = old_enable_signal;


	// Moving to a child of the full path, just append crumbs (preserve view cache)
	} else if (_breadcrumbs.size() > 0 && (path.is_child_of(_full_path))) {
		string postfix = path.substr(_full_path.length());
		while (postfix.length() > 0) {
			const string name = postfix.substr(0, postfix.find("/"));
			cerr << "NAME: " << name << endl;
			_full_path = _full_path.base() + name;
			BreadCrumb* but = create_crumb(_full_path, view);
			pack_end(*but, false, false, 1);
			_breadcrumbs.push_back(but);
			but->show();
			if (postfix.find("/") == string::npos)
				break;
			else
				postfix = postfix.substr(postfix.find("/")+1);
		}
		
		for (std::list<BreadCrumb*>::iterator i = _breadcrumbs.begin(); i != _breadcrumbs.end(); ++i)
			(*i)->set_active(false);
		_breadcrumbs.back()->set_active(true);


	// Rebuild from scratch
	// Getting here is bad unless absolutely necessary, since the PatchView cache is lost
	} else {

		_full_path = path;
		_active_path = path;

		// Empty existing breadcrumbs
		for (std::list<BreadCrumb*>::iterator i = _breadcrumbs.begin(); i != _breadcrumbs.end(); ++i)
			remove(**i);
		_breadcrumbs.clear();

		// Add root
		BreadCrumb* but = create_crumb("/", view);
		pack_start(*but, false, false, 1);
		_breadcrumbs.push_front(but);
		but->set_active(but->path() == _active_path);

		// Add the others
		while (path != "/") {
			BreadCrumb* but = create_crumb(path, view);
			pack_start(*but, false, false, 1);
			_breadcrumbs.push_front(but);
			but->set_active(but->path() == _active_path);
			path = path.parent();
		}
	}

	_enable_signal = old_enable_signal;
}


/** Create a new crumb, assigning it a reference to @a view if their paths
 * match, otherwise ignoring @a view.
 */
BreadCrumb*
BreadCrumbBox::create_crumb(const Path&           path,
                            CountedPtr<PatchView> view)
{
	BreadCrumb* but = manage(new BreadCrumb(path,
			(view && path == view->patch()->path()) ? view : CountedPtr<PatchView>()));
	
	but->signal_toggled().connect(sigc::bind(sigc::mem_fun(
				this, &BreadCrumbBox::breadcrumb_clicked), but));

	return but;
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
			signal_patch_selected.emit(crumb->path(), crumb->view());
			if (crumb->path() != _active_path)
				crumb->set_active(false);
		}
		_enable_signal = true;
	}
}


void
BreadCrumbBox::object_removed(const Path& path)
{
	for (std::list<BreadCrumb*>::iterator i = _breadcrumbs.begin(); i != _breadcrumbs.end(); ++i) {
		if ((*i)->path() == path) {
			// Remove all crumbs after the removed one (inclusive)
			for (std::list<BreadCrumb*>::iterator j = i; j != _breadcrumbs.end(); ) {
				BreadCrumb* bc = *j;
				j = _breadcrumbs.erase(j);
				remove(*bc);
			}
			break;
		}
	}
}


void
BreadCrumbBox::object_renamed(const Path& old_path, const Path& new_path)
{
	for (std::list<BreadCrumb*>::iterator i = _breadcrumbs.begin(); i != _breadcrumbs.end(); ++i) {
		if ((*i)->path() == old_path)
			(*i)->set_path(new_path);
	}
}


} // namespace Ingenuity

