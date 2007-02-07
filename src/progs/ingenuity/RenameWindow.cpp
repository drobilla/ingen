/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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
#include "Store.h"
#include "App.h"
#include "ModelEngineInterface.h"
using std::string;

namespace Ingenuity {


RenameWindow::RenameWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade_xml)
: Gtk::Window(cobject)
{
	glade_xml->get_widget("rename_name_entry", _name_entry);
	glade_xml->get_widget("rename_message_label", _message_label);
	glade_xml->get_widget("rename_cancel_button", _cancel_button);
	glade_xml->get_widget("rename_ok_button", _ok_button);

	_name_entry->signal_changed().connect(sigc::mem_fun(this, &RenameWindow::name_changed));
	_cancel_button->signal_clicked().connect(sigc::mem_fun(this, &RenameWindow::cancel_clicked));
	_ok_button->signal_clicked().connect(sigc::mem_fun(this, &RenameWindow::ok_clicked));

	_ok_button->property_sensitive() = false;
}


/** Set the object this window is renaming.
 * This function MUST be called before using this object in any way.
 */
void
RenameWindow::set_object(SharedPtr<ObjectModel> object)
{
	_object = object;
	_name_entry->set_text(object->path().name());
}


/** Called every time the user types into the name input box.
 * Used to display warning messages, and enable/disable the rename button.
 */
void
RenameWindow::name_changed()
{
	assert(_name_entry);
	assert(_message_label);
	assert(_object);
	assert(_object->parent());

	string name = _name_entry->get_text();
	if (name.find("/") != string::npos) {
		_message_label->set_text("Name may not contain '/'");
		_ok_button->property_sensitive() = false;
	//} else if (_object->parent()->patch_model()->get_node(name) != NULL) {
	} else if (App::instance().store()->object(_object->parent()->path().base() + name)) {
		_message_label->set_text("An object already exists with that name.");
		_ok_button->property_sensitive() = false;
	} else if (name.length() == 0) {
		_message_label->set_text("");
		_ok_button->property_sensitive() = false;
	} else {
		_message_label->set_text("");
		_ok_button->property_sensitive() = true;
	}	
}


void
RenameWindow::cancel_clicked()
{
	cout << "cancel\n";
	_name_entry->set_text("");
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
	string name = _name_entry->get_text();
	assert(name.length() > 0);
	assert(name.find("/") == string::npos);

	App::instance().engine()->rename(_object->path(), name);

	hide();
}


} // namespace Ingenuity
