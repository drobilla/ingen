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

#include "PatchDescriptionWindow.h"
#include <string>
#include "PatchModel.h"

namespace OmGtk {
using std::string;


PatchDescriptionWindow::PatchDescriptionWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade_xml)
: Gtk::Window(cobject)
{
	glade_xml->get_widget("description_author_entry", m_author_entry);
	glade_xml->get_widget("description_description_textview", m_textview);
	glade_xml->get_widget("description_cancel_button", m_cancel_button);
	glade_xml->get_widget("description_ok_button", m_ok_button);

	m_cancel_button->signal_clicked().connect(sigc::mem_fun(this, &PatchDescriptionWindow::cancel_clicked));
	m_ok_button->signal_clicked().connect(sigc::mem_fun(this, &PatchDescriptionWindow::ok_clicked));
}


/** Set the patch model this description is for.
 *
 * This function is a "post-constructor" - it MUST be called before using
 * the window in any way.
 */
void
PatchDescriptionWindow::patch_model(PatchModel* patch_model)
{
	property_title() = patch_model->path() + " Properties";
	m_patch_model = patch_model;
	m_author_entry->set_text(m_patch_model->get_metadata("author"));
	m_textview->get_buffer()->set_text(m_patch_model->get_metadata("description"));
}


void
PatchDescriptionWindow::cancel_clicked()
{
	m_author_entry->set_text(m_patch_model->get_metadata("author"));
	m_textview->get_buffer()->set_text(m_patch_model->get_metadata("description"));
	hide();
}


void
PatchDescriptionWindow::ok_clicked()
{
	m_patch_model->set_metadata("author", m_author_entry->get_text());
	m_patch_model->set_metadata("description", m_textview->get_buffer()->get_text());
	hide();
}



} // namespace OmGtk
