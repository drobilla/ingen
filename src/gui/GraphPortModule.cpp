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

using namespace std;

namespace Ingen {

using namespace Client;

namespace GUI {

GraphPortModule::GraphPortModule(GraphCanvas&                       canvas,
                                 SharedPtr<const Client::PortModel> model)
	: Ganv::Module(canvas, "", 0, 0, false) // FIXME: coords?
	, _model(model)
{
	assert(model);

	assert(PtrCast<const GraphModel>(model->parent()));

	set_stacked(model->polyphonic());

	model->signal_property().connect(
		sigc::mem_fun(this, &GraphPortModule::property_changed));

	signal_moved().connect(
		sigc::mem_fun(this, &GraphPortModule::store_location));
}

GraphPortModule*
GraphPortModule::create(GraphCanvas&               canvas,
                        SharedPtr<const PortModel> model,
                        bool                       human)
{
	GraphPortModule* ret  = new GraphPortModule(canvas, model);
	Port*            port = Port::create(canvas.app(), *ret, model, human, true);

	ret->set_port(port);

	for (Resource::Properties::const_iterator m = model->properties().begin();
	     m != model->properties().end(); ++m)
		ret->property_changed(m->first, m->second);

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

	const Raul::Atom x(app().forge().make(static_cast<float>(ax)));
	const Raul::Atom y(app().forge().make(static_cast<float>(ay)));

	if (x != _model->get_property(uris.ingen_canvasX) ||
	    y != _model->get_property(uris.ingen_canvasY))
	{
		Resource::Properties remove;
		remove.insert(make_pair(uris.ingen_canvasX, uris.wildcard));
		remove.insert(make_pair(uris.ingen_canvasY, uris.wildcard));
		Resource::Properties add;
		add.insert(make_pair(uris.ingen_canvasX,
		                     Resource::Property(x, Resource::INTERNAL)));
		add.insert(make_pair(uris.ingen_canvasY,
		                     Resource::Property(y, Resource::INTERNAL)));
		app().interface()->delta(_model->uri(), remove, add);
	}
}

void
GraphPortModule::show_human_names(bool b)
{
	const URIs&       uris = app().uris();
	const Raul::Atom& name = _model->get_property(uris.lv2_name);
	if (b && name.type() == uris.forge.String) {
		set_name(name.get_string());
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
GraphPortModule::property_changed(const Raul::URI& key, const Raul::Atom& value)
{
	const URIs& uris = app().uris();
	if (value.type() == uris.forge.Float) {
		if (key == uris.ingen_canvasX) {
			move_to(value.get_float(), get_y());
		} else if (key == uris.ingen_canvasY) {
			move_to(get_x(), value.get_float());
		}
	} else if (value.type() == uris.forge.String) {
		if (key == uris.lv2_name &&
		    app().world()->conf().option("human-names").get_bool()) {
			set_name(value.get_string());
		} else if (key == uris.lv2_symbol &&
		           !app().world()->conf().option("human-names").get_bool()) {
			set_name(value.get_string());
		}
	} else if (value.type() == uris.forge.Bool) {
		if (key == uris.ingen_polyphonic) {
			set_stacked(value.get_bool());
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
