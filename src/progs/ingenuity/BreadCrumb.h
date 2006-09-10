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

#ifndef BREADCRUMB_H
#define BREADCRUMB_H

// FIXME: remove
#include <iostream>
using std::cerr; using std::endl;

#include <gtkmm.h>
#include "util/Path.h"

namespace Ingenuity {


/** Breadcrumb button in a PatchWindow.
 *
 * \ingroup Ingenuity
 */
class BreadCrumb : public Gtk::ToggleButton
{
public:
	BreadCrumb(const Path& path)
		: m_path(path)
	{
		set_border_width(0);
		set_path(path);
		show_all();
	}

	void set_path(const Path& path)
	{
		remove();
		const string text = (path == "/") ? "/" : path.name();
		Gtk::Label* lab = manage(new Gtk::Label(text));
		lab->set_padding(0, 0);
		lab->show();
		add(*lab);
	}

	const Path& path() { return m_path; }
	
private:
	Path m_path;
};

} // namespace Ingenuity

#endif // BREADCRUMB_H
