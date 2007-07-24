/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#include "BreadCrumbBox.hpp"
#include "BreadCrumb.hpp"
namespace Ingen {
namespace GUI {


BreadCrumbBox::BreadCrumbBox()
: Gtk::HBox()
, _active_path("/")
, _full_path("/")
, _enable_signal(true)
{
}


SharedPtr<PatchView>
BreadCrumbBox::view(const Path& path)
{
	for (std::list<BreadCrumb*>::const_iterator i = _breadcrumbs.begin(); i != _breadcrumbs.end(); ++i)
		if ((*i)->path() == path)
			return (*i)->view();

	return SharedPtr<PatchView>();
}


/** Sets up the crumbs to display @a path.
 *
 * If @a path is already part of the shown path, it will be selected and the 
 * children preserved.
 */
void
BreadCrumbBox::build(Path path, SharedPtr<PatchView> view)
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

		string suffix = path.substr(_full_path.length());
		while (suffix.length() > 0) {
			if (suffix[0] == '/')
				suffix = suffix.substr(1);
			const string name = suffix.substr(0, suffix.find("/"));
			_full_path = _full_path.base() + name;
			BreadCrumb* but = create_crumb(_full_path, view);
			pack_start(*but, false, false, 1);
			_breadcrumbs.push_back(but);
			but->show();
			if (suffix.find("/") == string::npos)
				break;
			else
				suffix = suffix.substr(suffix.find("/")+1);
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
		BreadCrumb* root_but = create_crumb("/", view);
		pack_start(*root_but, false, false, 1);
		_breadcrumbs.push_front(root_but);
		root_but->set_active(root_but->path() == _active_path);

		Path working_path = "/";
		string suffix = path.substr(1);
		while (suffix.length() > 0) {
			if (suffix[0] == '/')
				suffix = suffix.substr(1);
			const string name = suffix.substr(0, suffix.find("/"));
			working_path = working_path.base() + name;
			BreadCrumb* but = create_crumb(working_path, view);
			pack_start(*but, false, false, 1);
			_breadcrumbs.push_back(but);
			but->set_active(working_path == _active_path);
			but->show();
			if (suffix.find("/") == string::npos)
				break;
			else
				suffix = suffix.substr(suffix.find("/")+1);
		}
	}
		
	_enable_signal = old_enable_signal;
}


/** Create a new crumb, assigning it a reference to @a view if their paths
 * match, otherwise ignoring @a view.
 */
BreadCrumb*
BreadCrumbBox::create_crumb(const Path&           path,
                            SharedPtr<PatchView> view)
{
	BreadCrumb* but = manage(new BreadCrumb(path,
			(view && path == view->patch()->path()) ? view : SharedPtr<PatchView>()));
	
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
			// Tried to turn off the current active button, bad user, no cookie
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


} // namespace GUI
} // namespace Ingen

