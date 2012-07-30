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

#include <math.h>

#include "ingen/Interface.hpp"
#include "ingen/client/PatchModel.hpp"
#include "ingen/client/PortModel.hpp"
#include "raul/SharedPtr.hpp"

#include "App.hpp"
#include "PortMenu.hpp"
#include "WindowFactory.hpp"

namespace Ingen {

using namespace Client;
using namespace Shared;

namespace GUI {

PortMenu::PortMenu(BaseObjectType*                   cobject,
                   const Glib::RefPtr<Gtk::Builder>& xml)
	: ObjectMenu(cobject, xml)
	, _is_patch_port(false)
{
	xml->get_widget("object_menu", _port_menu);
	xml->get_widget("port_set_min_menuitem", _set_min_menuitem);
	xml->get_widget("port_set_max_menuitem", _set_max_menuitem);
	xml->get_widget("port_reset_range_menuitem", _reset_range_menuitem);
	xml->get_widget("port_expose_menuitem", _expose_menuitem);
}

void
PortMenu::init(App& app, SharedPtr<const PortModel> port, bool is_patch_port)
{
	const URIs& uris = app.uris();

	ObjectMenu::init(app, port);
	_is_patch_port = is_patch_port;

	_set_min_menuitem->signal_activate().connect(
		sigc::mem_fun(this, &PortMenu::on_menu_set_min));

	_set_max_menuitem->signal_activate().connect(
		sigc::mem_fun(this, &PortMenu::on_menu_set_max));

	_reset_range_menuitem->signal_activate().connect(
		sigc::mem_fun(this, &PortMenu::on_menu_reset_range));

	_expose_menuitem->signal_activate().connect(
		sigc::mem_fun(this, &PortMenu::on_menu_expose));

	const bool is_control  = app.can_control(port.get()) && port->is_numeric();
	const bool is_on_patch = PtrCast<PatchModel>(port->parent());

	if (!_is_patch_port) {
		_polyphonic_menuitem->set_sensitive(false);
		_rename_menuitem->set_sensitive(false);
		_destroy_menuitem->set_sensitive(false);
	}

	if (port->is_a(uris.atom_AtomPort)) {
		_polyphonic_menuitem->hide();
	}

	_reset_range_menuitem->set_visible(is_control && !is_on_patch);
	_set_max_menuitem->set_visible(is_control);
	_set_min_menuitem->set_visible(is_control);
	_expose_menuitem->set_visible(!is_on_patch);
	_learn_menuitem->set_visible(is_control);
	_unlearn_menuitem->set_visible(is_control);

	_enable_signal = true;
}

void
PortMenu::on_menu_disconnect()
{
	if (_is_patch_port) {
		_app->interface()->disconnect_all(
				_object->parent()->path(), _object->path());
	} else {
		_app->interface()->disconnect_all(
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
		_app->interface()->set_property(_object->path(), uris.lv2_minimum, value);
}

void
PortMenu::on_menu_set_max()
{
	const URIs&                uris  = _app->uris();
	SharedPtr<const PortModel> model = PtrCast<const PortModel>(_object);
	const Raul::Atom&          value = model->get_property(uris.ingen_value);
	if (value.is_valid())
		_app->interface()->set_property(_object->path(), uris.lv2_maximum, value);
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
		_app->interface()->set_property(_object->path(),
		                                uris.lv2_minimum,
		                                _app->forge().make(min));

	if (!std::isnan(max))
		_app->interface()->set_property(_object->path(),
		                                uris.lv2_maximum,
		                                _app->forge().make(max));
}

void
PortMenu::on_menu_expose()
{
	const URIs&                uris = _app->uris();
	SharedPtr<const PortModel> port = PtrCast<const PortModel>(_object);
	SharedPtr<const NodeModel> node = PtrCast<const NodeModel>(port->parent());

	const std::string label = node->label() + " " + node->port_label(port);
	const Raul::Path  path  = node->path().str() + "_" + port->symbol().c_str();

	Ingen::Resource r(*_object.get());
	r.remove_property(uris.lv2_index, uris.wildcard);
	r.set_property(uris.lv2_symbol, _app->forge().alloc(path.symbol()));
	r.set_property(uris.lv2_name, _app->forge().alloc(label.c_str()));

	// TODO: Pretty kludgey coordinates
	const float node_x = node->get_property(uris.ingen_canvasX).get_float();
	const float node_y = node->get_property(uris.ingen_canvasY).get_float();
	const float x_off  = (label.length() * 16.0f) * (port->is_input() ? -1 : 1);
	const float y_off  = port->index() * 32.0f;
	r.set_property(uris.ingen_canvasX, _app->forge().make(node_x + x_off));
	r.set_property(uris.ingen_canvasY, _app->forge().make(node_y + y_off));

	_app->interface()->put(path, r.properties());

	if (port->is_input()) {
		_app->interface()->connect(path, _object->path());
	} else {
		_app->interface()->connect(_object->path(), path);
	}
}

} // namespace GUI
} // namespace Ingen

