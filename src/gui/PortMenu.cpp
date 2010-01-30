/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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
#include "raul/SharedPtr.hpp"
#include "interface/EngineInterface.hpp"
#include "client/PatchModel.hpp"
#include "client/PortModel.hpp"
#include "App.hpp"
#include "PortMenu.hpp"
#include "WindowFactory.hpp"

namespace Ingen {
namespace GUI {


PortMenu::PortMenu(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
	: ObjectMenu(cobject, xml)
	, _patch_port(NULL)
{
	xml->get_widget("port_menu", _port_menu);
	xml->get_widget("port_set_min_menuitem", _set_min_menuitem);
	xml->get_widget("port_set_max_menuitem", _set_max_menuitem);
	xml->get_widget("port_reset_range_menuitem", _reset_range_menuitem);
}


void
PortMenu::init(SharedPtr<PortModel> port, bool patch_port)
{
	ObjectMenu::init(port);
	_patch_port = patch_port;

	_set_min_menuitem->signal_activate().connect(sigc::mem_fun(this,
			&PortMenu::on_menu_set_min));

	_set_max_menuitem->signal_activate().connect(sigc::mem_fun(this,
			&PortMenu::on_menu_set_max));

	_reset_range_menuitem->signal_activate().connect(sigc::mem_fun(this,
			&PortMenu::on_menu_reset_range));

	if ( ! PtrCast<PatchModel>(port->parent()) ) {
		_polyphonic_menuitem->set_sensitive(false);
		_rename_menuitem->hide();
		_destroy_menuitem->hide();
	}

	if (port->type() == PortType::EVENTS)
		_polyphonic_menuitem->hide();

	if (port->type() == PortType::CONTROL) {
		_learn_menuitem->show();

		items().push_front(Gtk::Menu_Helpers::SeparatorElem());

		_port_menu->remove(*_reset_range_menuitem);
		insert(*_reset_range_menuitem, 0);

		_port_menu->remove(*_set_max_menuitem);
		insert(*_set_max_menuitem, 0);

		_port_menu->remove(*_set_min_menuitem);
		insert(*_set_min_menuitem, 0);
	}

	_enable_signal = true;
}


void
PortMenu::on_menu_disconnect()
{
	if (_patch_port) {
		App::instance().engine()->disconnect_all(
				_object->parent()->path(), _object->path());
	} else {
		App::instance().engine()->disconnect_all(
				_object->parent()->path().parent(), _object->path());
	}
}


void
PortMenu::on_menu_set_min()
{
	SharedPtr<PortModel> model = PtrCast<PortModel>(_object);
	const Raul::Atom&    value = model->get_property("ingen:value");
	std::cout << model->path() << " SET MIN " << value << std::endl;
	if (value.is_valid())
		App::instance().engine()->set_property(_object->path(), "lv2:minimum", value);
}


void
PortMenu::on_menu_set_max()
{
	SharedPtr<PortModel> model = PtrCast<PortModel>(_object);
	const Raul::Atom&    value = model->get_property("ingen:value");
	if (value.is_valid())
		App::instance().engine()->set_property(_object->path(), "lv2:maximum", value);
}


void
PortMenu::on_menu_reset_range()
{
	SharedPtr<PortModel> model  = PtrCast<PortModel>(_object);
	SharedPtr<NodeModel> parent = PtrCast<NodeModel>(_object->parent());

	float min, max;
	parent->default_port_value_range(model, min, max);

	if (!isnan(min))
		App::instance().engine()->set_property(_object->path(), "lv2:minimum", min);

	if (!isnan(max))
		App::instance().engine()->set_property(_object->path(), "lv2:maximum", max);
}


} // namespace GUI
} // namespace Ingen

