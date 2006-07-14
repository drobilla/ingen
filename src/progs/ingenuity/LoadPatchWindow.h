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

#ifndef LOADPATCHWINDOW_H
#define LOADPATCHWINDOW_H

#include "PluginModel.h"

#include <libglademm/xml.h>
#include <gtkmm.h>

namespace OmGtk {
	
class PatchController;


/** 'Load Patch' window.
 *
 * Loaded by glade as a derived object.  Used for both "Load" and "Load Into"
 * operations (the radio button state should be changed with the provided
 * methods prior to presenting this window).
 *
 * This is not for loading subpatches.  See @a LoadSubpatchWindow for that.
 *
 * \ingroup OmGtk
 */
class LoadPatchWindow : public Gtk::FileChooserDialog
{
public:
	LoadPatchWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml);

	void patch_controller(PatchController* pc);

	void set_replace() { m_replace = true; }
	void set_merge()   { m_replace = false; }

protected:
	void on_show();

private:
	void poly_from_file_selected();
	void poly_from_user_selected();
	void ok_clicked();
	void cancel_clicked();

	PatchController* m_patch_controller;
	bool             m_replace;

	Gtk::RadioButton* m_poly_from_current_radio;
	Gtk::RadioButton* m_poly_from_file_radio;
	Gtk::RadioButton* m_poly_from_user_radio;
	Gtk::SpinButton*  m_poly_spinbutton;
	Gtk::Button*      m_ok_button;
	Gtk::Button*      m_cancel_button;
};
 

} // namespace OmGtk

#endif // LOADPATCHWINDOW_H
