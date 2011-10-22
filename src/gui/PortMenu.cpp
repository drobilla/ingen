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

#include <math.h>
#include <gtkmm.h>
#include "raul/SharedPtr.hpp"
#include "ingen/ServerInterface.hpp"
#include "ingen/shared/LV2URIMap.hpp"
#include "ingen/client/PatchModel.hpp"
#include "ingen/client/PortModel.hpp"
#include "App.hpp"
#include "PortMenu.hpp"
#include "WindowFactory.hpp"

namespace Ingen {
namespace GUI {

PortMenu::PortMenu(BaseObjectType*                   cobject,
                   const Glib::RefPtr<Gtk::Builder>& xml)
	: ObjectMenu(cobject, xml)
	, _patch_port(NULL)
{
	xml->get_widget("object_menu", _port_menu);
	xml->get_widget("port_set_min_menuitem", _set_min_menuitem);
	xml->get_widget("port_set_max_menuitem", _set_max_menuitem);
	xml->get_widget("port_reset_range_menuitem", _reset_range_menuitem);
}

void
PortMenu::init(App& app, SharedPtr<const PortModel> port, bool patch_port)
{
	const URIs& uris = app.uris();

	ObjectMenu::init(app, port);
	_patch_port = patch_port;

	_set_min_menuitem->signal_activate().connect(sigc::mem_fun(this,
			&PortMenu::on_menu_set_min));

	_set_max_menuitem->signal_activate().connect(sigc::mem_fun(this,
			&PortMenu::on_menu_set_max));

	_reset_range_menuitem->signal_activate().connect(sigc::mem_fun(this,
			&PortMenu::on_menu_reset_range));

	if ( ! PtrCast<PatchModel>(port->parent()) ) {
		_polyphonic_menuitem->set_sensitive(false);
		_rename_menuitem->set_sensitive(false);
		_destroy_menuitem->set_sensitive(false);
	}

	if (port->is_a(uris.ev_EventPort))
		_polyphonic_menuitem->hide();

	const bool is_control = app.can_control(port.get())
		&& port->is_numeric();

	_reset_range_menuitem->set_visible(true);
	_set_max_menuitem->set_visible(true);
	_set_min_menuitem->set_visible(true);

	_reset_range_menuitem->set_sensitive(is_control);
	_set_max_menuitem->set_sensitive(is_control);
	_set_min_menuitem->set_sensitive(is_control);

	if (is_control) {
		_learn_menuitem->show();
		_unlearn_menuitem->show();
	}

	_enable_signal = true;
}

void
PortMenu::on_menu_disconnect()
{
	if (_patch_port) {
		_app->engine()->disconnect_all(
				_object->parent()->path(), _object->path());
	} else {
		_app->engine()->disconnect_all(
				_object->parent()->path().parent(), _object->path());
	}
}

void
PortMenu::on_menu_set_min()
{
	const URIs&                uris  = _app->uris();
	SharedPtr<const PortModel> model = PtrCast<const PortModel>(_object);
	const Raul::Atom&          value = model->get_property(uris.ingen_value);
	if (value.is_valid())
		_app->engine()->set_property(_object->path(), uris.lv2_minimum, value);
}

void
PortMenu::on_menu_set_max()
{
	const URIs&                uris  = _app->uris();
	SharedPtr<const PortModel> model = PtrCast<const PortModel>(_object);
	const Raul::Atom&          value = model->get_property(uris.ingen_value);
	if (value.is_valid())
		_app->engine()->set_property(_object->path(), uris.lv2_maximum, value);
}

void
PortMenu::on_menu_reset_range()
{
	const URIs&                uris  = _app->uris();
	SharedPtr<const PortModel> model  = PtrCast<const PortModel>(_object);
	SharedPtr<const NodeModel> parent = PtrCast<const NodeModel>(_object->parent());

	float min, max;
	parent->default_port_value_range(model, min, max);

	if (!std::isnan(min))
		_app->engine()->set_property(_object->path(), uris.lv2_minimum, min);

	if (!std::isnan(max))
		_app->engine()->set_property(_object->path(), uris.lv2_maximum, max);
}

} // namespace GUI
} // namespace Ingen

