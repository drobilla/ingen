/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#include <utility>
#include <gtkmm.h>
#include "interface/EngineInterface.hpp"
#include "shared/LV2URIMap.hpp"
#include "client/ObjectModel.hpp"
#include "App.hpp"
#include "ObjectMenu.hpp"
#include "WindowFactory.hpp"

using namespace Raul;

namespace Ingen {
namespace GUI {


ObjectMenu::ObjectMenu(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
	: Gtk::Menu(cobject)
	, _enable_signal(false)
	, _polyphonic_menuitem(NULL)
	, _disconnect_menuitem(NULL)
	, _rename_menuitem(NULL)
	, _destroy_menuitem(NULL)
	, _properties_menuitem(NULL)
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
ObjectMenu::init(SharedPtr<ObjectModel> object)
{
	_object = object;

	App& app = App::instance();

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
			sigc::mem_fun(app.window_factory(), &WindowFactory::present_rename),
			object));

	_destroy_menuitem->signal_activate().connect(
			sigc::mem_fun(this, &ObjectMenu::on_menu_destroy));

	_properties_menuitem->signal_activate().connect(
			sigc::mem_fun(this, &ObjectMenu::on_menu_properties));

	object->signal_property.connect(sigc::mem_fun(this, &ObjectMenu::property_changed));

	_learn_menuitem->hide();
	_unlearn_menuitem->hide();

	_enable_signal = true;
}


void
ObjectMenu::on_menu_learn()
{
	App::instance().engine()->set_property(_object->path(),
			App::instance().uris().ingen_controlBinding,
			App::instance().uris().wildcard);
}


void
ObjectMenu::on_menu_unlearn()
{
	Resource::Properties remove;
	remove.insert(std::make_pair(
			App::instance().uris().ingen_controlBinding,
			App::instance().uris().wildcard));
	App::instance().engine()->delta(_object->path(), remove, Resource::Properties());
}


void
ObjectMenu::on_menu_polyphonic()
{
	if (_enable_signal)
		App::instance().engine()->set_property(_object->path(),
				App::instance().uris().ingen_polyphonic,
				bool(_polyphonic_menuitem->get_active()));
}


void
ObjectMenu::property_changed(const URI& predicate, const Atom& value)
{
	const LV2URIMap& uris = App::instance().uris();
	_enable_signal = false;
	if (predicate == uris.ingen_polyphonic && value.type() == Atom::BOOL)
		_polyphonic_menuitem->set_active(value.get_bool());
	_enable_signal = true;
}


void
ObjectMenu::on_menu_destroy()
{
	App::instance().engine()->del(_object->path());
}


void
ObjectMenu::on_menu_properties()
{
	App::instance().window_factory()->present_properties(_object);
}


} // namespace GUI
} // namespace Ingen

