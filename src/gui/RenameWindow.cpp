/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cassert>
#include <string>

#include "ingen/Interface.hpp"
#include "ingen/client/ClientStore.hpp"
#include "ingen/client/ObjectModel.hpp"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#include "App.hpp"
#include "RenameWindow.hpp"

using namespace std;

namespace Ingen {

using namespace Client;

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
RenameWindow::set_object(SPtr<const ObjectModel> object)
{
	_object = object;
	_symbol_entry->set_text(object->path().symbol());
	const Atom& name_atom = object->get_property(_app->uris().lv2_name);
	_label_entry->set_text(
		(name_atom.type() == _app->forge().String) ? name_atom.ptr<char>() : "");
}

void
RenameWindow::present(SPtr<const ObjectModel> object)
{
	set_object(object);
	_symbol_entry->grab_focus();
	Gtk::Window::present();
}

void
RenameWindow::values_changed()
{
	const string& symbol = _symbol_entry->get_text();
	if (!Raul::Symbol::is_valid(symbol)) {
		_message_label->set_text("Invalid symbol");
		_ok_button->property_sensitive() = false;
	} else if (_object->symbol() != symbol &&
	           _app->store()->object(
		           _object->parent()->path().child(Raul::Symbol(symbol)))) {
		_message_label->set_text("An object already exists with that path");
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
	const URIs&   uris       = _app->uris();
	const string& symbol_str = _symbol_entry->get_text();
	const string& label      = _label_entry->get_text();
	Raul::Path    path       = _object->path();
	const Atom&   name_atom  = _object->get_property(uris.lv2_name);

	if (!label.empty() && (!name_atom.is_valid() || label != name_atom.ptr<char>())) {
		_app->set_property(Node::path_to_uri(path),
		                   uris.lv2_name,
		                   _app->forge().alloc(label));
	}

	if (Raul::Symbol::is_valid(symbol_str)) {
		const Raul::Symbol symbol(symbol_str);
		if (symbol != _object->symbol()) {
			path = _object->path().parent().child(symbol);
			_app->interface()->move(_object->path(), path);
		}
	}

	hide();
}

} // namespace GUI
} // namespace Ingen
