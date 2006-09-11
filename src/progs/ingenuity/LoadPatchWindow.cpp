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

#include "LoadPatchWindow.h"
#include <sys/types.h>
#include <dirent.h>
#include "App.h"
#include "Configuration.h"
#include "PatchController.h"
#include "PatchModel.h"
#include "ModelEngineInterface.h"
#include "Loader.h"

namespace Ingenuity {


LoadPatchWindow::LoadPatchWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
: Gtk::FileChooserDialog(cobject),
  m_replace(true)
{
	xml->get_widget("load_patch_poly_from_current_radio", m_poly_from_current_radio);
	xml->get_widget("load_patch_poly_from_file_radio", m_poly_from_file_radio);
	xml->get_widget("load_patch_poly_from_user_radio", m_poly_from_user_radio);
	xml->get_widget("load_patch_poly_spinbutton", m_poly_spinbutton);
	xml->get_widget("load_patch_ok_button", m_ok_button);
	xml->get_widget("load_patch_cancel_button", m_cancel_button);
	
	m_poly_from_current_radio->signal_toggled().connect(sigc::mem_fun(this, &LoadPatchWindow::poly_from_file_selected));
	m_poly_from_file_radio->signal_toggled().connect(sigc::mem_fun(this, &LoadPatchWindow::poly_from_file_selected));
	m_poly_from_user_radio->signal_toggled().connect(sigc::mem_fun(this, &LoadPatchWindow::poly_from_user_selected));
	m_ok_button->signal_clicked().connect(sigc::mem_fun(this, &LoadPatchWindow::ok_clicked));
	m_cancel_button->signal_clicked().connect(sigc::mem_fun(this, &LoadPatchWindow::cancel_clicked));
	
	m_poly_from_current_radio->set_active(true);

	Gtk::FileFilter filt;
	filt.add_pattern("*.om");
	filt.set_name("Om patch files (*.om)");
	set_filter(filt);

	// Add global examples directory to "shortcut folders" (bookmarks)
	string examples_dir = PKGDATADIR;
	examples_dir.append("/patches");
	DIR* d = opendir(examples_dir.c_str());
	if (d != NULL)
		add_shortcut_folder(examples_dir);
}


/** Sets the patch controller for this window and initializes everything.
 *
 * This function MUST be called before using the window in any way!
 */
void
LoadPatchWindow::set_patch(CountedPtr<PatchController> pc)
{
	m_patch_controller = pc;
}


void
LoadPatchWindow::on_show()
{
	if (App::instance().configuration()->patch_folder().length() > 0)
		set_current_folder(App::instance().configuration()->patch_folder());
	Gtk::FileChooserDialog::on_show();
}


///// Event Handlers //////


void
LoadPatchWindow::poly_from_file_selected()
{
	m_poly_spinbutton->property_sensitive() = false;
}


void
LoadPatchWindow::poly_from_user_selected()
{
	m_poly_spinbutton->property_sensitive() = true;
}


void
LoadPatchWindow::ok_clicked()
{
	// These values are interpreted by load_patch() as "not defined", ie load from file
	string name = "";
	int    poly = 0;
	
	if (m_poly_from_user_radio->get_active())
		poly = m_poly_spinbutton->get_value_as_int();
	
	if (m_replace)
		App::instance().engine()->clear_patch(m_patch_controller->model()->path());

	CountedPtr<PatchModel> pm(new PatchModel(m_patch_controller->model()->path(), poly));
	pm->filename(get_filename());
	pm->set_metadata("filename", get_filename());
	pm->set_parent(m_patch_controller->patch_model()->parent());
	//App::instance().engine()->push_added_patch(pm);
	App::instance().loader()->load_patch(pm, true, true);
	
	hide();
}			


void
LoadPatchWindow::cancel_clicked()
{
	hide();
}


} // namespace Ingenuity
