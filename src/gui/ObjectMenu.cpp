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

#include "ObjectMenu.hpp"

#include "App.hpp"
#include "WindowFactory.hpp"

#include "ingen/Atom.hpp"
#include "ingen/Forge.hpp"
#include "ingen/Interface.hpp"
#include "ingen/Properties.hpp"
#include "ingen/URIs.hpp"
#include "ingen/client/ObjectModel.hpp"

#include <glibmm/refptr.h>
#include <glibmm/signalproxy.h>
#include <gtkmm/builder.h>
#include <gtkmm/checkmenuitem.h>
#include <gtkmm/menuitem.h>
#include <gtkmm/separatormenuitem.h>
#include <sigc++/adaptors/bind.h>
#include <sigc++/functors/mem_fun.h>
#include <sigc++/signal.h>

#include <cstdint>
#include <memory>

namespace ingen {
namespace gui {

ObjectMenu::ObjectMenu(BaseObjectType*                   cobject,
                       const Glib::RefPtr<Gtk::Builder>& xml)
	: Gtk::Menu(cobject)
{
	xml->get_widget("object_learn_menuitem", _learn_menuitem);
	xml->get_widget("object_unlearn_menuitem", _unlearn_menuitem);
	xml->get_widget("object_polyphonic_menuitem", _polyphonic_menuitem);
	xml->get_widget("object_disconnect_menuitem", _disconnect_menuitem);
	xml->get_widget("object_rename_menuitem", _rename_menuitem);
	xml->get_widget("object_destroy_menuitem", _destroy_menuitem);
	xml->get_widget("object_properties_menuitem", _properties_menuitem);
	xml->get_widget("object_menu_separator", _separator_menuitem);
}

void
ObjectMenu::init(App&                                              app,
                 const std::shared_ptr<const client::ObjectModel>& object)
{
	_app = &app;
	_object = object;

	_polyphonic_menuitem->signal_toggled().connect(
		sigc::mem_fun(this, &ObjectMenu::on_menu_polyphonic));

	_polyphonic_menuitem->set_active(object->polyphonic());

	_learn_menuitem->signal_activate().connect(
		sigc::mem_fun(this, &ObjectMenu::on_menu_learn));

	_unlearn_menuitem->signal_activate().connect(
		sigc::mem_fun(this, &ObjectMenu::on_menu_unlearn));

	_disconnect_menuitem->signal_activate().connect(
		sigc::mem_fun(this, &ObjectMenu::on_menu_disconnect));

	_rename_menuitem->signal_activate().connect(
		sigc::bind(sigc::mem_fun(_app->window_factory(), &WindowFactory::present_rename),
		           object));

	_destroy_menuitem->signal_activate().connect(
		sigc::mem_fun(this, &ObjectMenu::on_menu_destroy));

	_properties_menuitem->signal_activate().connect(
		sigc::mem_fun(this, &ObjectMenu::on_menu_properties));

	object->signal_property().connect(sigc::mem_fun(this, &ObjectMenu::property_changed));

	_learn_menuitem->hide();
	_unlearn_menuitem->hide();

	_enable_signal = true;
}

void
ObjectMenu::on_menu_learn()
{
	_app->interface()->set_property(_object->uri(),
	                                _app->uris().midi_binding,
	                                _app->uris().patch_wildcard.urid_atom());
}

void
ObjectMenu::on_menu_unlearn()
{
	Properties remove;
	remove.emplace(_app->uris().midi_binding,
	               Property(_app->uris().patch_wildcard));
	_app->interface()->delta(_object->uri(), remove, Properties());
}

void
ObjectMenu::on_menu_polyphonic()
{
	if (_enable_signal) {
		_app->set_property(
			_object->uri(),
			_app->uris().ingen_polyphonic,
			_app->forge().make(bool(_polyphonic_menuitem->get_active())));
	}
}

void
ObjectMenu::property_changed(const URI& predicate, const Atom& value)
{
	const URIs& uris = _app->uris();
	_enable_signal = false;
	if (predicate == uris.ingen_polyphonic && value.type() == uris.forge.Bool) {
		_polyphonic_menuitem->set_active(value.get<int32_t>());
	}
	_enable_signal = true;
}

void
ObjectMenu::on_menu_destroy()
{
	_app->interface()->del(_object->uri());
}

void
ObjectMenu::on_menu_properties()
{
	_app->window_factory()->present_properties(_object);
}

} // namespace gui
} // namespace ingen
