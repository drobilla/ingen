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

#include <gtkmm.h>
#include "interface/EngineInterface.hpp"
#include "client/ObjectModel.hpp"
#include "App.hpp"
#include "ObjectMenu.hpp"
#include "WindowFactory.hpp"

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
	
	_disconnect_menuitem->signal_activate().connect(
			sigc::mem_fun(this, &ObjectMenu::on_menu_disconnect));

	_rename_menuitem->signal_activate().connect(sigc::bind(
			sigc::mem_fun(app.window_factory(), &WindowFactory::present_rename),
			object));
	
	_destroy_menuitem->signal_activate().connect(
			sigc::mem_fun(this, &ObjectMenu::on_menu_destroy));
	
	_properties_menuitem->signal_activate().connect(
			sigc::mem_fun(this, &ObjectMenu::on_menu_properties));

	object->signal_polyphonic.connect(sigc::mem_fun(this, &ObjectMenu::polyphonic_changed));

	_enable_signal = true;
}


void
ObjectMenu::on_menu_polyphonic()
{
	if (_enable_signal)
		App::instance().engine()->set_polyphonic(_object->path(), _polyphonic_menuitem->get_active());
}


void
ObjectMenu::polyphonic_changed(bool polyphonic)
{
	_enable_signal = false;
	_polyphonic_menuitem->set_active(polyphonic);
	_enable_signal = true;
}


void
ObjectMenu::on_menu_disconnect()
{
	App::instance().engine()->disconnect_all(_object->path());
}

	
void
ObjectMenu::on_menu_destroy()
{
	App::instance().engine()->destroy(_object->path());
}


void
ObjectMenu::on_menu_properties()
{
	App::instance().window_factory()->present_properties(_object);
}


} // namespace GUI
} // namespace Ingen

