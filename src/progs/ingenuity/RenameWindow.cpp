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

#include "RenameWindow.h"
#include <cassert>
#include <string>
#include "ObjectModel.h"
#include "GtkObjectController.h"
#include "Store.h"
#include "App.h"
#include "ModelEngineInterface.h"
using std::string;

namespace Ingenuity {


RenameWindow::RenameWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade_xml)
: Gtk::Window(cobject)
{
	glade_xml->get_widget("rename_name_entry", m_name_entry);
	glade_xml->get_widget("rename_message_label", m_message_label);
	glade_xml->get_widget("rename_cancel_button", m_cancel_button);
	glade_xml->get_widget("rename_ok_button", m_ok_button);

	m_name_entry->signal_changed().connect(sigc::mem_fun(this, &RenameWindow::name_changed));
	m_cancel_button->signal_clicked().connect(sigc::mem_fun(this, &RenameWindow::cancel_clicked));
	m_ok_button->signal_clicked().connect(sigc::mem_fun(this, &RenameWindow::ok_clicked));

	m_ok_button->property_sensitive() = false;
}


/** Set the object this window is renaming.
 * This function MUST be called before using this object in any way.
 */
void
RenameWindow::set_object(GtkObjectController* object)
{
	m_object = object;
	m_name_entry->set_text(object->path().name());
}


/** Called every time the user types into the name input box.
 * Used to display warning messages, and enable/disable the rename button.
 */
void
RenameWindow::name_changed()
{
	assert(m_name_entry);
	assert(m_message_label);
	assert(m_object->model());
	assert(m_object->model()->parent());

	string name = m_name_entry->get_text();
	if (name.find("/") != string::npos) {
		m_message_label->set_text("Name may not contain '/'");
		m_ok_button->property_sensitive() = false;
	//} else if (m_object->parent()->patch_model()->get_node(name) != NULL) {
	} else if (App::instance().store()->object(m_object->model()->parent()->path().base() + name)) {
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
RenameWindow::cancel_clicked()
{
	cout << "cancel\n";
	m_name_entry->set_text("");
	hide();
}


/** Rename the object.
 *
 * It shouldn't be possible for this to be called with an invalid name set
 * (since the Rename button should be deactivated).  This is just shinification
 * though - the engine will handle invalid names gracefully.
 */
void
RenameWindow::ok_clicked()
{
	string name = m_name_entry->get_text();
	assert(name.length() > 0);
	assert(name.find("/") == string::npos);

	App::instance().engine()->rename(m_object->model()->path(), name);

	hide();
}


} // namespace Ingenuity
