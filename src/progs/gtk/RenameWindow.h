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

#ifndef RENAMEWINDOW_H
#define RENAMEWINDOW_H

#include <gtkmm.h>
#include <libglademm.h>


namespace OmGtk {

class GtkObjectController;


/** 'New Patch' Window.
 *
 * Loaded by libglade as a derived object.
 *
 * \ingroup OmGtk
 */
class RenameWindow : public Gtk::Window
{
public:
	RenameWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& refGlade);

	void set_object(GtkObjectController* object);

private:
	void name_changed();
	void cancel_clicked();
	void ok_clicked();
	
	GtkObjectController* m_object;

	Gtk::Entry*      m_name_entry;
	Gtk::Label*      m_message_label;
	Gtk::Button*     m_cancel_button;
	Gtk::Button*     m_ok_button;
};

} // namespace OmGtk

#endif // RENAMEWINDOW_H
