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

#include "App.h"
#include "ModelEngineInterface.h"
#include "NewSubpatchWindow.h"
#include "NodeModel.h"
#include "PatchModel.h"
#include "PatchView.h"

namespace Ingenuity {


NewSubpatchWindow::NewSubpatchWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
: Gtk::Window(cobject),
  m_new_module_x(0),
  m_new_module_y(0)
{
	xml->get_widget("new_subpatch_name_entry", m_name_entry);
	xml->get_widget("new_subpatch_message_label", m_message_label);
	xml->get_widget("new_subpatch_polyphony_spinbutton", m_poly_spinbutton);
	xml->get_widget("new_subpatch_ok_button", m_ok_button);
	xml->get_widget("new_subpatch_cancel_button", m_cancel_button);

	m_name_entry->signal_changed().connect(sigc::mem_fun(this, &NewSubpatchWindow::name_changed));
	m_ok_button->signal_clicked().connect(sigc::mem_fun(this, &NewSubpatchWindow::ok_clicked));
	m_cancel_button->signal_clicked().connect(sigc::mem_fun(this, &NewSubpatchWindow::cancel_clicked));
	
	m_ok_button->property_sensitive() = false;
}

void
NewSubpatchWindow::present(CountedPtr<PatchModel> patch, MetadataMap data)
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
NewSubpatchWindow::set_patch(CountedPtr<PatchModel> patch)
{
	m_patch = patch;
}


/** Called every time the user types into the name input box.
 * Used to display warning messages, and enable/disable the OK button.
 */
void
NewSubpatchWindow::name_changed()
{
	string name = m_name_entry->get_text();
	if (!Path::is_valid_name(name)) {
		m_message_label->set_text("Name contains invalid characters.");
		m_ok_button->property_sensitive() = false;
	} else if (m_patch->get_node(name)) {
		m_message_label->set_text("An object already exists with that name.");
		m_ok_button->property_sensitive() = false;
	} else if (name.length() == 0) {
		m_message_label->set_text("");
		m_ok_button->property_sensitive() = false;
	} else {
		m_message_label->set_text("");
		m_ok_button->property_sensitive() = true;
	}	
}


void
NewSubpatchWindow::ok_clicked()
{
	cerr << "FIXME: new subpatch\n";
#if 0
	PatchModel* pm = new PatchModel(
		m_patch->path().base() + m_name_entry->get_text(),
		m_poly_spinbutton->get_value_as_int());

	if (m_new_module_x == 0 && m_new_module_y == 0) {
		throw; // FIXME
		//m_patch_controller->get_view()->canvas()->get_new_module_location(
		//	m_new_module_x, m_new_module_y);
	}

	// FIXME: necessary?
	//pm->set_parent(m_patch);
	pm->set_metadata("module-x", (float)m_new_module_x);
	pm->set_metadata("module-y", (float)m_new_module_y);
	App::instance().engine()->create_patch_from_model(pm);
#endif
	hide();
}			


void
NewSubpatchWindow::cancel_clicked()
{
	hide();
}


} // namespace Ingenuity
