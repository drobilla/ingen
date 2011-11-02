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

#include <cassert>
#include <utility>
#include "PatchPortModule.hpp"
#include "ingen/ServerInterface.hpp"
#include "ingen/shared/LV2URIMap.hpp"
#include "ingen/client/PatchModel.hpp"
#include "ingen/client/NodeModel.hpp"
#include "App.hpp"
#include "Configuration.hpp"
#include "WidgetFactory.hpp"
#include "PatchCanvas.hpp"
#include "PatchWindow.hpp"
#include "Port.hpp"
#include "PortMenu.hpp"
#include "RenameWindow.hpp"
#include "WindowFactory.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace GUI {

PatchPortModule::PatchPortModule(PatchCanvas&               canvas,
                                 SharedPtr<const PortModel> model)
	: FlowCanvas::Module(canvas, "", 0, 0, false) // FIXME: coords?
	, _model(model)
{
	assert(model);

	assert(PtrCast<const PatchModel>(model->parent()));

	set_stacked_border(model->polyphonic());

	model->signal_property().connect(
		sigc::mem_fun(this, &PatchPortModule::property_changed));
}

PatchPortModule*
PatchPortModule::create(PatchCanvas&               canvas,
                        SharedPtr<const PortModel> model,
                        bool                       human)
{
	PatchPortModule* ret  = new PatchPortModule(canvas, model);
	Port*            port = Port::create(canvas.app(), *ret, model, human, true);

	ret->set_port(port);

	for (GraphObject::Properties::const_iterator m = model->properties().begin();
	     m != model->properties().end(); ++m)
		ret->property_changed(m->first, m->second);

	return ret;
}

App&
PatchPortModule::app() const
{
	return ((PatchCanvas*)canvas())->app();
}

bool
PatchPortModule::show_menu(GdkEventButton* ev)
{
	std::cout << "PPM SHOW MENU" << std::endl;
	return _port->show_menu(ev);
}

void
PatchPortModule::store_location()
{
	const Atom x(static_cast<float>(property_x()));
	const Atom y(static_cast<float>(property_y()));

	const URIs& uris = app().uris();

	const Atom& existing_x = _model->get_property(uris.ingen_canvas_x);
	const Atom& existing_y = _model->get_property(uris.ingen_canvas_y);

	if (x != existing_x && y != existing_y) {
		Resource::Properties remove;
		remove.insert(make_pair(uris.ingen_canvas_x, uris.wildcard));
		remove.insert(make_pair(uris.ingen_canvas_y, uris.wildcard));
		Resource::Properties add;
		add.insert(make_pair(uris.ingen_canvas_x,
		                     Resource::Property(x, Resource::INTERNAL)));
		add.insert(make_pair(uris.ingen_canvas_y,
		                     Resource::Property(y, Resource::INTERNAL)));
		app().engine()->delta(_model->path(), remove, add);
	}
}

void
PatchPortModule::show_human_names(bool b)
{
	const URIs& uris = app().uris();
	const Atom& name = _model->get_property(uris.lv2_name);
	if (b && name.type() == Atom::STRING)
		set_name(name.get_string());
	else
		set_name(_model->symbol().c_str());
}

void
PatchPortModule::set_name(const std::string& n)
{
	_port->set_name(n);
	_must_resize = true;
}

void
PatchPortModule::property_changed(const URI& key, const Atom& value)
{
	const URIs& uris = app().uris();
	switch (value.type()) {
	case Atom::FLOAT:
		if (key == uris.ingen_canvas_x) {
			move_to(value.get_float(), property_y());
		} else if (key == uris.ingen_canvas_y) {
			move_to(property_x(), value.get_float());
		}
		break;
	case Atom::STRING:
		if (key == uris.lv2_name
		    && app().configuration()->name_style() == Configuration::HUMAN) {
			set_name(value.get_string());
		} else if (key == uris.lv2_symbol
		           && app().configuration()->name_style() == Configuration::PATH) {
			set_name(value.get_string());
		}
		break;
	case Atom::BOOL:
		if (key == uris.ingen_polyphonic) {
			set_stacked_border(value.get_bool());
		} else if (key == uris.ingen_selected) {
			if (value.get_bool() != selected()) {
				if (value.get_bool()) {
					_canvas->select_item(this);
				} else {
					_canvas->unselect_item(this);
				}
			}
		}
	default: break;
	}
}

void
PatchPortModule::set_selected(bool b)
{
	if (b != selected()) {
		Module::set_selected(b);
		if (app().signal())
			app().engine()->set_property(_model->path(),
			                             app().uris().ingen_selected, b);
	}
}

} // namespace GUI
} // namespace Ingen
