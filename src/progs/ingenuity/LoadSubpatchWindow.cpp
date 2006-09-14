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

#include "LoadSubpatchWindow.h"
#include <sys/types.h>
#include <dirent.h>
#include <cassert>
#include "App.h"
#include "PatchView.h"
#include "NodeModel.h"
#include "PatchModel.h"
#include "Configuration.h"
#include "ModelEngineInterface.h"
#include "Loader.h"

namespace Ingenuity {


LoadSubpatchWindow::LoadSubpatchWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
: Gtk::FileChooserDialog(cobject)
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


void
LoadSubpatchWindow::present(CountedPtr<PatchModel> patch, MetadataMap data)
{
	set_patch(patch);
	m_initial_data = data;
	Gtk::Window::present();
}


/** Sets the patch controller for this window and initializes everything.
 *
 * This function MUST be called before using the window in any way!
 */
void
LoadSubpatchWindow::set_patch(CountedPtr<PatchModel> patch)
{
	m_patch = patch;

	char temp_buf[4];
	snprintf(temp_buf, 4, "%zd", patch->poly());
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
	assert(m_patch);
	
	const string filename = get_filename();

	// FIXME
	string name = filename.substr(filename.find_last_of("/")+1);
	int    poly = 1;
	
	if (m_name_from_user_radio->get_active())
		name = m_name_entry->get_text();

	if (m_poly_from_user_radio->get_active())
		poly = m_poly_spinbutton->get_value_as_int();
	else if (m_poly_from_parent_radio->get_active())
		poly = m_patch->poly();

	/*CountedPtr<PatchModel> pm(new PatchModel(m_patch->path().base() + name, poly));
	pm->filename(filename);
	
	pm->set_metadata("module-x", Atom((float)m_new_module_x));
	pm->set_metadata("module-y", Atom((float)m_new_module_y));
	
	App::instance().loader()->load_patch(pm, true, false);

	App::instance().configuration()->set_patch_folder(pm->filename().substr(0, pm->filename().find_last_of("/")));
	*/
	cerr << "FIXME: load subpatch" << endl;

	hide();
}			


void
LoadSubpatchWindow::cancel_clicked()
{
	hide();
}


} // namespace Ingenuity
