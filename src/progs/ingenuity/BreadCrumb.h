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

#ifndef BREADCRUMB_H
#define BREADCRUMB_H

#include <gtkmm.h>
#include <raul/Path.h>
#include <raul/SharedPtr.h>
#include "PatchView.h"

namespace Ingenuity {


/** Breadcrumb button in a PatchWindow.
 *
 * Each Breadcrumb stores a reference to a PatchView for quick switching.
 * So, the amount of allocated PatchViews at a given time is equal to the
 * number of visible breadcrumbs (which is the perfect cache for GUI
 * responsiveness balanced with mem consumption).
 *
 * \ingroup Ingenuity
 */
class BreadCrumb : public Gtk::ToggleButton
{
public:
	BreadCrumb(const Path& path, SharedPtr<PatchView> view = SharedPtr<PatchView>())
		: _path(path)
		, _view(view)
	{
		assert( !view || view->patch()->path() == path);
		set_border_width(0);
		set_path(path);
		show_all();
	}

	void set_view(SharedPtr<PatchView> view) {
		assert( !view || view->patch()->path() == _path);
		_view = view;
	}

	const Path&           path() const { return _path; }
	SharedPtr<PatchView> view() const { return _view; }
	
	void set_path(const Path& path)
	{
		remove();
		const string text = (path == "/") ? "/" : path.name();
		Gtk::Label* lab = manage(new Gtk::Label(text));
		lab->set_padding(0, 0);
		lab->show();
		add(*lab);

		if (_view && _view->patch()->path() != path)
			_view.reset();
	}

private:
	Path                 _path;
	SharedPtr<PatchView> _view;
};

} // namespace Ingenuity

#endif // BREADCRUMB_H
