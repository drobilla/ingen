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

#include <string>
#include "client/PatchModel.hpp"
#include "PatchPropertiesWindow.hpp"

namespace Ingen {
namespace GUI {
using std::string;


PatchPropertiesWindow::PatchPropertiesWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade_xml)
: Gtk::Window(cobject)
{
	glade_xml->get_widget("properties_author_entry", _author_entry);
	glade_xml->get_widget("properties_description_textview", _textview);
	glade_xml->get_widget("properties_cancel_button", _cancel_button);
	glade_xml->get_widget("properties_ok_button", _ok_button);

	_cancel_button->signal_clicked().connect(sigc::mem_fun(this, &PatchPropertiesWindow::cancel_clicked));
	_ok_button->signal_clicked().connect(sigc::mem_fun(this, &PatchPropertiesWindow::ok_clicked));
}


/** Set the patch model this description is for.
 *
 * This function is a "post-constructor" - it MUST be called before using
 * the window in any way.
 */
void
PatchPropertiesWindow::set_patch(SharedPtr<PatchModel> patch_model)
{
	property_title() = patch_model->path() + " Properties";
	_patch_model = patch_model;
	
	const Atom& author_atom = _patch_model->get_metadata("author");
	_author_entry->set_text(
		(author_atom.type() == Atom::STRING) ? author_atom.get_string() : "" );

	const Atom& desc_atom = _patch_model->get_metadata("description");
	_textview->get_buffer()->set_text(
		(desc_atom.type() == Atom::STRING) ? desc_atom.get_string() : "" );
}


void
PatchPropertiesWindow::cancel_clicked()
{
	const Atom& author_atom = _patch_model->get_metadata("author");
	_author_entry->set_text(
		(author_atom.type() == Atom::STRING) ? author_atom.get_string() : "" );

	const Atom& desc_atom = _patch_model->get_metadata("description");
	_textview->get_buffer()->set_text(
		(desc_atom.type() == Atom::STRING) ? desc_atom.get_string() : "" );
	
	hide();
}


void
PatchPropertiesWindow::ok_clicked()
{
	cerr << "FIXME: properties\n";

	//m_patch_model->set_metadata("author", Atom(_author_entry->get_text().c_str()));
	//m_patch_model->set_metadata("description", Atom(_textview->get_buffer()->get_text().c_str()));
	hide();
}



} // namespace GUI
} // namespace Ingen
