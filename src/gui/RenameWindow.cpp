/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
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

#include <cassert>
#include <string>

#include "ingen/shared/LV2URIMap.hpp"

#include "ingen/ServerInterface.hpp"
#include "ingen/client/ObjectModel.hpp"
#include "ingen/client/ClientStore.hpp"

#include "App.hpp"
#include "RenameWindow.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace GUI {

RenameWindow::RenameWindow(BaseObjectType*                   cobject,
                           const Glib::RefPtr<Gtk::Builder>& xml)
	: Window(cobject)
{
	xml->get_widget("rename_symbol_entry", _symbol_entry);
	xml->get_widget("rename_label_entry", _label_entry);
	xml->get_widget("rename_message_label", _message_label);
	xml->get_widget("rename_cancel_button", _cancel_button);
	xml->get_widget("rename_ok_button", _ok_button);

	_symbol_entry->signal_changed().connect(
		sigc::mem_fun(this, &RenameWindow::values_changed));
	_label_entry->signal_changed().connect(
		sigc::mem_fun(this, &RenameWindow::values_changed));
	_cancel_button->signal_clicked().connect(
		sigc::mem_fun(this, &RenameWindow::cancel_clicked));
	_ok_button->signal_clicked().connect(
		sigc::mem_fun(this, &RenameWindow::ok_clicked));

	_ok_button->property_sensitive() = false;
}

/** Set the object this window is renaming.
 * This function MUST be called before using this object in any way.
 */
void
RenameWindow::set_object(SharedPtr<const ObjectModel> object)
{
	_object = object;
	_symbol_entry->set_text(object->path().symbol());
	const Atom& name_atom = object->get_property("http://lv2plug.in/ns/lv2core#name");
	_label_entry->set_text(
		(name_atom.type() == Atom::STRING) ? name_atom.get_string() : "");
}

void
RenameWindow::present(SharedPtr<const ObjectModel> object)
{
	set_object(object);
	_symbol_entry->grab_focus();
	Gtk::Window::present();
}

void
RenameWindow::values_changed()
{
	const string& symbol = _symbol_entry->get_text();
	const string& label  = _label_entry->get_text();
	if (symbol.length() == 0) {
		_message_label->set_text("Symbol must be at least 1 character");
		_ok_button->property_sensitive() = false;
	} else if (!Path::is_valid_name(symbol)) {
		_message_label->set_text("Symbol contains invalid characters");
		_ok_button->property_sensitive() = false;
	} else if (App::instance().store()->object(_object->parent()->path().child(symbol))) {
		_message_label->set_text("An object already exists with that path");
		_ok_button->property_sensitive() = false;
	} else if (label.empty()) {
		_message_label->set_text("Label must be at least 1 character");
		_ok_button->property_sensitive() = false;
	} else {
		_message_label->set_text("");
		_ok_button->property_sensitive() = true;
	}
}

void
RenameWindow::cancel_clicked()
{
	_symbol_entry->set_text("");
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
	const Shared::URIs& uris = App::instance().uris();

	const string& symbol_str = _symbol_entry->get_text();
	const string& label      = _label_entry->get_text();
	Path          path       = _object->path();
	const Atom&   name_atom  = _object->get_property(uris.lv2_name);

	if (Symbol::is_valid(symbol_str)) {
		const Symbol& symbol(symbol_str);
		if (symbol != _object->symbol()) {
			path = _object->path().parent().child(symbol);
			App::instance().engine()->move(_object->path(), path);
		}
	}

	if (!label.empty() && (!name_atom.is_valid() || label != name_atom.get_string())) {
		App::instance().engine()->set_property(path, uris.lv2_name, Atom(label));
	}

	hide();
}

} // namespace GUI
} // namespace Ingen
