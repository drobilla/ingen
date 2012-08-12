/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <utility>

#include "ingen/Interface.hpp"
#include "ingen/client/ObjectModel.hpp"

#include "App.hpp"
#include "ObjectMenu.hpp"
#include "WidgetFactory.hpp"
#include "WindowFactory.hpp"

using namespace Raul;

namespace Ingen {

using namespace Client;

namespace GUI {

ObjectMenu::ObjectMenu(BaseObjectType*                   cobject,
                       const Glib::RefPtr<Gtk::Builder>& xml)
	: Gtk::Menu(cobject)
	, _app(NULL)
	, _polyphonic_menuitem(NULL)
	, _disconnect_menuitem(NULL)
	, _rename_menuitem(NULL)
	, _destroy_menuitem(NULL)
	, _properties_menuitem(NULL)
	, _enable_signal(false)
{
	xml->get_widget("object_learn_menuitem", _learn_menuitem);
	xml->get_widget("object_unlearn_menuitem", _unlearn_menuitem);
	xml->get_widget("object_polyphonic_menuitem", _polyphonic_menuitem);
	xml->get_widget("object_disconnect_menuitem", _disconnect_menuitem);
	xml->get_widget("object_rename_menuitem", _rename_menuitem);
	xml->get_widget("object_destroy_menuitem", _destroy_menuitem);
	xml->get_widget("object_properties_menuitem", _properties_menuitem);
}

void
ObjectMenu::init(App& app, SharedPtr<const ObjectModel> object)
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

	_rename_menuitem->signal_activate().connect(sigc::bind(
			sigc::mem_fun(_app->window_factory(), &WindowFactory::present_rename),
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
			_app->uris().ingen_controlBinding,
			_app->uris().wildcard);
}

void
ObjectMenu::on_menu_unlearn()
{
	Resource::Properties remove;
	remove.insert(std::make_pair(
			_app->uris().ingen_controlBinding,
			_app->uris().wildcard));
	_app->interface()->delta(_object->uri(), remove, Resource::Properties());
}

void
ObjectMenu::on_menu_polyphonic()
{
	if (_enable_signal)
		_app->interface()->set_property(
			_object->uri(),
			_app->uris().ingen_polyphonic,
			_app->forge().make(bool(_polyphonic_menuitem->get_active())));
}

void
ObjectMenu::property_changed(const URI& predicate, const Atom& value)
{
	const URIs& uris = _app->uris();
	_enable_signal = false;
	if (predicate == uris.ingen_polyphonic && value.type() == uris.forge.Bool)
		_polyphonic_menuitem->set_active(value.get_bool());
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

} // namespace GUI
} // namespace Ingen

