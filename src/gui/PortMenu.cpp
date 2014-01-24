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
#include "ingen/client/GraphModel.hpp"
#include "ingen/client/PortModel.hpp"
#include "ingen/types.hpp"

#include "App.hpp"
#include "PortMenu.hpp"
#include "WindowFactory.hpp"

namespace Ingen {

using namespace Client;

namespace GUI {

PortMenu::PortMenu(BaseObjectType*                   cobject,
                   const Glib::RefPtr<Gtk::Builder>& xml)
	: ObjectMenu(cobject, xml)
	, _is_graph_port(false)
{
	xml->get_widget("object_menu", _port_menu);
	xml->get_widget("port_set_min_menuitem", _set_min_menuitem);
	xml->get_widget("port_set_max_menuitem", _set_max_menuitem);
	xml->get_widget("port_reset_range_menuitem", _reset_range_menuitem);
	xml->get_widget("port_expose_menuitem", _expose_menuitem);
}

void
PortMenu::init(App& app, SPtr<const PortModel> port, bool is_graph_port)
{
	const URIs& uris = app.uris();

	ObjectMenu::init(app, port);
	_is_graph_port = is_graph_port;

	_set_min_menuitem->signal_activate().connect(
		sigc::mem_fun(this, &PortMenu::on_menu_set_min));

	_set_max_menuitem->signal_activate().connect(
		sigc::mem_fun(this, &PortMenu::on_menu_set_max));

	_reset_range_menuitem->signal_activate().connect(
		sigc::mem_fun(this, &PortMenu::on_menu_reset_range));

	_expose_menuitem->signal_activate().connect(
		sigc::mem_fun(this, &PortMenu::on_menu_expose));

	const bool is_control(app.can_control(port.get()) && port->is_numeric());
	const bool is_on_graph(dynamic_ptr_cast<GraphModel>(port->parent()));

	if (!_is_graph_port) {
		_polyphonic_menuitem->set_sensitive(false);
		_rename_menuitem->set_sensitive(false);
		_destroy_menuitem->set_sensitive(false);
	}

	if (port->is_a(uris.atom_AtomPort)) {
		_polyphonic_menuitem->hide();
	}

	_reset_range_menuitem->set_visible(is_control && !is_on_graph);
	_set_max_menuitem->set_visible(is_control);
	_set_min_menuitem->set_visible(is_control);
	_expose_menuitem->set_visible(!is_on_graph);
	_learn_menuitem->set_visible(is_control);
	_unlearn_menuitem->set_visible(is_control);

	_enable_signal = true;
}

void
PortMenu::on_menu_disconnect()
{
	if (_is_graph_port) {
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
	const URIs&           uris  = _app->uris();
	SPtr<const PortModel> model = dynamic_ptr_cast<const PortModel>(_object);
	const Atom&     value = model->get_property(uris.ingen_value);
	if (value.is_valid())
		_app->interface()->set_property(_object->uri(), uris.lv2_minimum, value);
}

void
PortMenu::on_menu_set_max()
{
	const URIs&           uris  = _app->uris();
	SPtr<const PortModel> model = dynamic_ptr_cast<const PortModel>(_object);
	const Atom&     value = model->get_property(uris.ingen_value);
	if (value.is_valid())
		_app->interface()->set_property(_object->uri(), uris.lv2_maximum, value);
}

void
PortMenu::on_menu_reset_range()
{
	const URIs&           uris  = _app->uris();
	SPtr<const PortModel> model = dynamic_ptr_cast<const PortModel>(_object);

	// Remove lv2:minimum and lv2:maximum properties
	Resource::Properties remove;
	remove.insert({uris.lv2_minimum, Resource::Property(uris.patch_wildcard)});
	remove.insert({uris.lv2_maximum, Resource::Property(uris.patch_wildcard)});
	_app->interface()->delta(_object->uri(), remove, Resource::Properties());
}

void
PortMenu::on_menu_expose()
{
	const URIs&            uris  = _app->uris();
	SPtr<const PortModel>  port  = dynamic_ptr_cast<const PortModel>(_object);
	SPtr<const BlockModel> block = dynamic_ptr_cast<const BlockModel>(port->parent());

	const std::string label = block->label() + " " + block->port_label(port);
	const Raul::Path  path  = Raul::Path(block->path() + Raul::Symbol("_" + port->symbol()));

	Ingen::Resource r(*_object.get());
	r.remove_property(uris.lv2_index, uris.patch_wildcard);
	r.set_property(uris.lv2_symbol, _app->forge().alloc(path.symbol()));
	r.set_property(uris.lv2_name, _app->forge().alloc(label.c_str()));

	// TODO: Pretty kludgey coordinates
	const float block_x = block->get_property(uris.ingen_canvasX).get<float>();
	const float block_y = block->get_property(uris.ingen_canvasY).get<float>();
	const float x_off   = (label.length() * 16.0f) * (port->is_input() ? -1 : 1);
	const float y_off   = port->index() * 32.0f;
	r.set_property(uris.ingen_canvasX, _app->forge().make(block_x + x_off));
	r.set_property(uris.ingen_canvasY, _app->forge().make(block_y + y_off));

	_app->interface()->put(Node::path_to_uri(path), r.properties());

	if (port->is_input()) {
		_app->interface()->connect(path, _object->path());
	} else {
		_app->interface()->connect(_object->path(), path);
	}
}

} // namespace GUI
} // namespace Ingen

