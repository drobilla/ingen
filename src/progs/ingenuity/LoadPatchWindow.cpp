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
#include <boost/optional/optional.hpp>
#include "App.h"
#include "Configuration.h"
#include "PatchModel.h"
#include "ModelEngineInterface.h"
#include "Loader.h"

using boost::optional;

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


void
LoadPatchWindow::present(SharedPtr<PatchModel> patch, MetadataMap data)
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
LoadPatchWindow::set_patch(SharedPtr<PatchModel> patch)
{
	m_patch = patch;
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
	// If unset load_patch will load values
	optional<const string&> name;
	optional<size_t> poly;
	
	if (m_poly_from_user_radio->get_active())
		poly = m_poly_spinbutton->get_value_as_int();
	
	if (m_replace)
		App::instance().engine()->clear_patch(m_patch->path());

	App::instance().loader()->load_patch(true, get_filename(), "/",
		m_initial_data, m_patch->parent()->path(), name, poly);
	
	hide();
}			


void
LoadPatchWindow::cancel_clicked()
{
	hide();
}


} // namespace Ingenuity
