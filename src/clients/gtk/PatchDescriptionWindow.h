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

#ifndef PATCHDESCRIPTIONWINDOW_H
#define PATCHDESCRIPTIONWINDOW_H

#include <string>
#include <gtkmm.h>
#include <libglademm/xml.h>
using std::string;

namespace LibOmClient { class PatchModel; }
using LibOmClient::PatchModel;

namespace OmGtk {
	

/** Patch Description Window.
 *
 * Loaded by libglade as a derived object.
 *
 * \ingroup OmGtk
 */
class PatchDescriptionWindow : public Gtk::Window
{
public:
	PatchDescriptionWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& refGlade);

	void patch_model(PatchModel* patch_model);
	
	void cancel_clicked();
	void ok_clicked();

private:
	PatchModel*    m_patch_model;

	Gtk::Entry*    m_author_entry;
	Gtk::TextView* m_textview;
	Gtk::Button*   m_cancel_button;
	Gtk::Button*   m_ok_button;
};


} // namespace OmGtk

#endif // PATCHDESCRIPTIONWINDOW_H
