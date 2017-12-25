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
#include <utility>

#include "ingen/Configuration.hpp"
#include "ingen/Interface.hpp"
#include "ingen/client/BlockModel.hpp"
#include "ingen/client/GraphModel.hpp"

#include "App.hpp"
#include "Style.hpp"
#include "GraphCanvas.hpp"
#include "GraphPortModule.hpp"
#include "GraphWindow.hpp"
#include "Port.hpp"
#include "PortMenu.hpp"
#include "RenameWindow.hpp"
#include "WidgetFactory.hpp"
#include "WindowFactory.hpp"

namespace Ingen {

using namespace Client;

namespace GUI {

GraphPortModule::GraphPortModule(GraphCanvas&                  canvas,
                                 SPtr<const Client::PortModel> model)
	: Ganv::Module(canvas, "", 0, 0, false) // FIXME: coords?
	, _model(model)
	, _port(nullptr)
{
	assert(model);

	assert(dynamic_ptr_cast<const GraphModel>(model->parent()));

	set_stacked(model->polyphonic());
	if (model->is_input() && !model->is_numeric()) {
		set_is_source(true);
	}

	model->signal_property().connect(
		sigc::mem_fun(this, &GraphPortModule::property_changed));

	signal_moved().connect(
		sigc::mem_fun(this, &GraphPortModule::store_location));
}

GraphPortModule*
GraphPortModule::create(GraphCanvas&          canvas,
                        SPtr<const PortModel> model)
{
	GraphPortModule* ret  = new GraphPortModule(canvas, model);
	Port*            port = Port::create(canvas.app(), *ret, model, true);

	ret->set_port(port);
	if (model->is_numeric()) {
		port->show_control();
	}

	for (const auto& p : model->properties()) {
		ret->property_changed(p.first, p.second);
	}

	return ret;
}

App&
GraphPortModule::app() const
{
	return ((GraphCanvas*)canvas())->app();
}

bool
GraphPortModule::show_menu(GdkEventButton* ev)
{
	return _port->show_menu(ev);
}

void
GraphPortModule::store_location(double ax, double ay)
{
	const URIs& uris = app().uris();

	const Atom x(app().forge().make(static_cast<float>(ax)));
	const Atom y(app().forge().make(static_cast<float>(ay)));

	if (x != _model->get_property(uris.ingen_canvasX) ||
	    y != _model->get_property(uris.ingen_canvasY))
	{
		app().interface()->put(
			_model->uri(),
			{{uris.ingen_canvasX, Property(x, Property::Graph::INTERNAL)},
			 {uris.ingen_canvasY, Property(y, Property::Graph::INTERNAL)}});
	}
}

void
GraphPortModule::show_human_names(bool b)
{
	const URIs& uris = app().uris();
	const Atom& name = _model->get_property(uris.lv2_name);
	if (b && name.type() == uris.forge.String) {
		set_name(name.ptr<char>());
	} else {
		set_name(_model->symbol().c_str());
	}
}

void
GraphPortModule::set_name(const std::string& n)
{
	_port->set_label(n.c_str());
}

void
GraphPortModule::property_changed(const Raul::URI& key, const Atom& value)
{
	const URIs& uris = app().uris();
	if (value.type() == uris.forge.Float) {
		if (key == uris.ingen_canvasX) {
			move_to(value.get<float>(), get_y());
		} else if (key == uris.ingen_canvasY) {
			move_to(get_x(), value.get<float>());
		}
	} else if (value.type() == uris.forge.String) {
		if (key == uris.lv2_name &&
		    app().world()->conf().option("human-names").get<int32_t>()) {
			set_name(value.ptr<char>());
		} else if (key == uris.lv2_symbol &&
		           !app().world()->conf().option("human-names").get<int32_t>()) {
			set_name(value.ptr<char>());
		}
	} else if (value.type() == uris.forge.Bool) {
		if (key == uris.ingen_polyphonic) {
			set_stacked(value.get<int32_t>());
		}
	}
}

void
GraphPortModule::set_selected(gboolean b)
{
	if (b != get_selected()) {
		Module::set_selected(b);
	}
}

} // namespace GUI
} // namespace Ingen
