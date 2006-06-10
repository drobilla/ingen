/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef BREADCRUMB_H
#define BREADCRUMB_H

#include <gtkmm.h>
#include "PatchWindow.h"
#include "PatchController.h"
#include "PatchModel.h"

namespace OmGtk {


/** Breadcrumb button in a PatchWindow.
 *
 * \ingroup OmGtk
 */
class BreadCrumb : public Gtk::ToggleButton
{
public:
	BreadCrumb(PatchWindow* window, PatchController* patch)
		: m_window(window),
		  m_patch(patch)
	{
		assert(patch != NULL);
		set_border_width(0);
		path(m_patch->path());
		signal_clicked().connect(sigc::bind(sigc::mem_fun(
			m_window, &PatchWindow::breadcrumb_clicked), this));
		show_all();
	}

	PatchController* patch() { return m_patch; }
	
	void path(const Path& path)
	{
		remove();
		const string text = (path == "/") ? "/" : path.name();
		Gtk::Label* lab = manage(new Gtk::Label(text));
		lab->set_padding(0, 0);
		lab->show();
		add(*lab);
	}
	
private:
	PatchWindow*     m_window;
	PatchController* m_patch;
};

} // namespace OmGtk

#endif // BREADCRUMB_H
