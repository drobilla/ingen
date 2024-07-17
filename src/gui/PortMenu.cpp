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

#include "PortMenu.hpp"

#include "App.hpp"
#include "ObjectMenu.hpp"

#include "ingen/Atom.hpp"
#include "ingen/Forge.hpp"
#include "ingen/Interface.hpp"
#include "ingen/Properties.hpp"
#include "ingen/Resource.hpp"
#include "ingen/URI.hpp"
#include "ingen/URIs.hpp"
#include "ingen/client/BlockModel.hpp"
#include "ingen/client/GraphModel.hpp" // IWYU pragma: keep
#include "ingen/client/ObjectModel.hpp"
#include "ingen/client/PortModel.hpp"
#include "ingen/paths.hpp"
#include "raul/Path.hpp"
#include "raul/Symbol.hpp"

#include <glibmm/refptr.h>
#include <gtkmm/builder.h>
#include <gtkmm/checkmenuitem.h>
#include <gtkmm/menu.h>
#include <gtkmm/menuitem.h>
#include <gtkmm/separatormenuitem.h>
#include <sigc++/functors/mem_fun.h>

#include <memory>
#include <string>

namespace ingen {

using client::BlockModel;
using client::GraphModel;
using client::PortModel;

namespace gui {

PortMenu::PortMenu(BaseObjectType*                   cobject,
                   const Glib::RefPtr<Gtk::Builder>& xml)
	: ObjectMenu(cobject, xml)
{
	xml->get_widget("object_menu", _port_menu);
	xml->get_widget("port_set_min_menuitem", _set_min_menuitem);
	xml->get_widget("port_set_max_menuitem", _set_max_menuitem);
	xml->get_widget("port_reset_range_menuitem", _reset_range_menuitem);
	xml->get_widget("port_expose_menuitem", _expose_menuitem);
}

void
PortMenu::init(App&                                    app,
               const std::shared_ptr<const PortModel>& port,
               bool                                    internal_graph_port)
{
	const URIs& uris = app.uris();

	ObjectMenu::init(app, port);
	_internal_graph_port = internal_graph_port;

	_set_min_menuitem->signal_activate().connect(
		sigc::mem_fun(this, &PortMenu::on_menu_set_min));

	_set_max_menuitem->signal_activate().connect(
		sigc::mem_fun(this, &PortMenu::on_menu_set_max));

	_reset_range_menuitem->signal_activate().connect(
		sigc::mem_fun(this, &PortMenu::on_menu_reset_range));

	_expose_menuitem->signal_activate().connect(
		sigc::mem_fun(this, &PortMenu::on_menu_expose));

	const bool is_control(app.can_control(port.get()) && port->is_numeric());
	const bool is_on_graph(std::dynamic_pointer_cast<GraphModel>(port->parent()));
	const bool is_input(port->is_input());

	if (!is_on_graph) {
		_polyphonic_menuitem->set_sensitive(false);
		_rename_menuitem->set_sensitive(false);
		_destroy_menuitem->set_sensitive(false);
	}

	if (port->is_a(uris.atom_AtomPort)) {
		_polyphonic_menuitem->hide();
	}

	_reset_range_menuitem->set_visible(is_input && is_control && !is_on_graph);
	_set_max_menuitem->set_visible(is_input && is_control);
	_set_min_menuitem->set_visible(is_input && is_control);
	_expose_menuitem->set_visible(!is_on_graph);
	_learn_menuitem->set_visible(is_input && is_control);
	_unlearn_menuitem->set_visible(is_input && is_control);

	if (!is_control && is_on_graph) {
		_separator_menuitem->hide();
	}

	_enable_signal = true;
}

void
PortMenu::on_menu_disconnect()
{
	if (_internal_graph_port) {
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
	const URIs& uris  = _app->uris();
	auto        model = std::dynamic_pointer_cast<const PortModel>(_object);
	const Atom& value = model->get_property(uris.ingen_value);
	if (value.is_valid()) {
		_app->set_property(_object->uri(), uris.lv2_minimum, value);
	}
}

void
PortMenu::on_menu_set_max()
{
	const URIs& uris  = _app->uris();
	auto        model = std::dynamic_pointer_cast<const PortModel>(_object);
	const Atom& value = model->get_property(uris.ingen_value);
	if (value.is_valid()) {
		_app->set_property(_object->uri(), uris.lv2_maximum, value);
	}
}

void
PortMenu::on_menu_reset_range()
{
	const URIs& uris  = _app->uris();
	auto        model = std::dynamic_pointer_cast<const PortModel>(_object);

	// Remove lv2:minimum and lv2:maximum properties
	Properties remove;
	remove.insert({uris.lv2_minimum, Property(uris.patch_wildcard)});
	remove.insert({uris.lv2_maximum, Property(uris.patch_wildcard)});
	_app->interface()->delta(_object->uri(), remove, Properties());
}

void
PortMenu::on_menu_expose()
{
	const URIs& uris = _app->uris();
	auto        port = std::dynamic_pointer_cast<const PortModel>(_object);
	auto block = std::dynamic_pointer_cast<const BlockModel>(port->parent());

	const std::string label = block->label() + " " + block->port_label(port);
	const auto        path  = raul::Path{block->path() + raul::Symbol("_" + port->symbol())};

	ingen::Resource r(*_object);
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

	_app->interface()->put(path_to_uri(path), r.properties());

	if (port->is_input()) {
		_app->interface()->connect(path, _object->path());
	} else {
		_app->interface()->connect(_object->path(), path);
	}
}

} // namespace gui
} // namespace ingen
