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
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "LoadSubpatchWindow.h"
#include <sys/types.h>
#include <dirent.h>
#include <cassert>
#include "App.h"
#include "PatchController.h"
#include "NodeModel.h"
#include "Controller.h"
#include "PatchModel.h"
#include "Configuration.h"

namespace OmGtk {


LoadSubpatchWindow::LoadSubpatchWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
: Gtk::FileChooserDialog(cobject),
  m_patch_controller(NULL),
  m_new_module_x(0),
  m_new_module_y(0)
{
	xml->get_widget("load_subpatch_name_from_file_radio", m_name_from_file_radio);
	xml->get_widget("load_subpatch_name_from_user_radio", m_name_from_user_radio);
	xml->get_widget("load_subpatch_name_entry", m_name_entry);
	xml->get_widget("load_subpatch_poly_from_file_radio", m_poly_from_file_radio);
	xml->get_widget("load_subpatch_poly_from_parent_radio", m_poly_from_parent_radio);
	xml->get_widget("load_subpatch_poly_from_user_radio", m_poly_from_user_radio);
	xml->get_widget("load_subpatch_poly_spinbutton", m_poly_spinbutton);
	xml->get_widget("load_subpatch_ok_button", m_ok_button);
	xml->get_widget("load_subpatch_cancel_button", m_cancel_button);

	m_name_from_file_radio->signal_toggled().connect(sigc::mem_fun(this, &LoadSubpatchWindow::disable_name_entry));
	m_name_from_user_radio->signal_toggled().connect(sigc::mem_fun(this, &LoadSubpatchWindow::enable_name_entry));
	m_poly_from_file_radio->signal_toggled().connect(sigc::mem_fun(this, &LoadSubpatchWindow::disable_poly_spinner));
	m_poly_from_parent_radio->signal_toggled().connect(sigc::mem_fun(this, &LoadSubpatchWindow::disable_poly_spinner));
	m_poly_from_user_radio->signal_toggled().connect(sigc::mem_fun(this, &LoadSubpatchWindow::enable_poly_spinner));
	m_ok_button->signal_clicked().connect(sigc::mem_fun(this, &LoadSubpatchWindow::ok_clicked));
	m_cancel_button->signal_clicked().connect(sigc::mem_fun(this, &LoadSubpatchWindow::cancel_clicked));

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
LoadSubpatchWindow::patch_controller(PatchController* pc)
{
	m_patch_controller = pc;

	char temp_buf[4];
	snprintf(temp_buf, 4, "%zd", pc->patch_model()->poly());
	Glib::ustring txt = "Same as parent (";
	txt.append(temp_buf).append(")");
	m_poly_from_parent_radio->set_label(txt);
}


void
LoadSubpatchWindow::on_show()
{
	if (App::instance().configuration()->patch_folder().length() > 0)
		set_current_folder(App::instance().configuration()->patch_folder());
	Gtk::FileChooserDialog::on_show();
}


///// Event Handlers //////



void
LoadSubpatchWindow::disable_name_entry()
{
	m_name_entry->property_sensitive() = false;
}


void
LoadSubpatchWindow::enable_name_entry()
{
	m_name_entry->property_sensitive() = true;
}


void
LoadSubpatchWindow::disable_poly_spinner()
{
	m_poly_spinbutton->property_sensitive() = false;
}


void
LoadSubpatchWindow::enable_poly_spinner()
{
	m_poly_spinbutton->property_sensitive() = true;
}


void
LoadSubpatchWindow::ok_clicked()
{
	assert(m_patch_controller != NULL);
	assert(m_patch_controller->model());
	
	// These values are interpreted by load_patch() as "not defined", ie load from file
	string name = "";
	int    poly = 0;
	
	if (m_name_from_user_radio->get_active())
		name = m_name_entry->get_text();

	if (m_poly_from_user_radio->get_active())
		poly = m_poly_spinbutton->get_value_as_int();
	else if (m_poly_from_parent_radio->get_active())
		poly = m_patch_controller->patch_model()->poly();

	if (m_new_module_x == 0 && m_new_module_y == 0) {
		m_patch_controller->get_new_module_location(
			m_new_module_x, m_new_module_y);
	}

	PatchModel* pm = new PatchModel(m_patch_controller->model()->base_path() + name, poly);
	pm->filename(get_filename());
	pm->set_parent(m_patch_controller->model().get());
	pm->x(m_new_module_x);
	pm->y(m_new_module_y);
	if (name == "")
		pm->set_path("");
	char temp_buf[16];
	snprintf(temp_buf, 16, "%d", m_new_module_x);
	pm->set_metadata("module-x", temp_buf);
	snprintf(temp_buf, 16, "%d", m_new_module_y);
	pm->set_metadata("module-y", temp_buf);
	Controller::instance().load_patch(pm);

	App::instance().configuration()->set_patch_folder(pm->filename().substr(0, pm->filename().find_last_of("/")));
	
	hide();
}			


void
LoadSubpatchWindow::cancel_clicked()
{
	hide();
}


} // namespace OmGtk
