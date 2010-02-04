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

#include <cassert>
#include <utility>
#include "PatchPortModule.hpp"
#include "interface/EngineInterface.hpp"
#include "shared/LV2URIMap.hpp"
#include "client/PatchModel.hpp"
#include "client/NodeModel.hpp"
#include "App.hpp"
#include "Configuration.hpp"
#include "GladeFactory.hpp"
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


PatchPortModule::PatchPortModule(boost::shared_ptr<PatchCanvas> canvas, SharedPtr<PortModel> model)
	: FlowCanvas::Module(canvas, "", 0, 0, false) // FIXME: coords?
	, _model(model)
{
	assert(canvas);
	assert(model);

	assert(PtrCast<PatchModel>(model->parent()));

	set_stacked_border(model->polyphonic());

	model->signal_property.connect(sigc::mem_fun(this, &PatchPortModule::set_property));
}


boost::shared_ptr<PatchPortModule>
PatchPortModule::create(boost::shared_ptr<PatchCanvas> canvas, SharedPtr<PortModel> model, bool human)
{
	boost::shared_ptr<PatchPortModule> ret(new PatchPortModule(canvas, model));
	boost::shared_ptr<Port> port(Port::create(ret, model, human, true));

	ret->add_port(port);
	ret->set_port(port);
	ret->set_menu(port->menu());

	for (GraphObject::Properties::const_iterator m = model->meta().properties().begin();
			m != model->meta().properties().end(); ++m)
		ret->set_property(m->first, m->second);

	for (GraphObject::Properties::const_iterator m = model->properties().begin();
			m != model->properties().end(); ++m)
		ret->set_property(m->first, m->second);

	ret->resize();
	return ret;
}


void
PatchPortModule::create_menu()
{
	Glib::RefPtr<Gnome::Glade::Xml> xml = GladeFactory::new_glade_reference();
	xml->get_widget_derived("object_menu", _menu);
	_menu->init(_model, true);

	set_menu(_menu);
}


void
PatchPortModule::store_location()
{
	const float x = static_cast<float>(property_x());
	const float y = static_cast<float>(property_y());

	const LV2URIMap& uris = App::instance().uris();

	const Atom& existing_x = _model->get_property(uris.ingenui_canvas_x);
	const Atom& existing_y = _model->get_property(uris.ingenui_canvas_y);

	if (existing_x.type() != Atom::FLOAT || existing_y.type() != Atom::FLOAT
			|| existing_x.get_float() != x || existing_y.get_float() != y) {
		Shared::Resource::Properties props;
		props.insert(make_pair(uris.ingenui_canvas_x, Atom(x)));
		props.insert(make_pair(uris.ingenui_canvas_y, Atom(y)));
		App::instance().engine()->put(_model->meta_uri(), props);
	}
}


void
PatchPortModule::show_human_names(bool b)
{
	const LV2URIMap& uris = App::instance().uris();
	const Atom& name = _model->get_property(uris.lv2_name);
	if (b && name.type() == Atom::STRING)
		set_name(name.get_string());
	else
		set_name(_model->symbol().c_str());

	resize();
}


void
PatchPortModule::set_name(const std::string& n)
{
	_port->set_name(n);
	Module::resize();
}


void
PatchPortModule::set_property(const URI& key, const Atom& value)
{
	const LV2URIMap& uris = App::instance().uris();
	switch (value.type()) {
	case Atom::FLOAT:
		if (key == uris.ingenui_canvas_x) {
			move_to(value.get_float(), property_y());
		} else if (key == uris.ingenui_canvas_y) {
			move_to(property_x(), value.get_float());
		}
		break;
	case Atom::STRING:
		if (key == uris.lv2_name
				&& App::instance().configuration()->name_style() == Configuration::HUMAN) {
			set_name(value.get_string());
		} else if (key == uris.lv2_symbol
				&& App::instance().configuration()->name_style() == Configuration::PATH) {
			set_name(value.get_string());
		}
		break;
	case Atom::BOOL:
		if (key == uris.ingen_polyphonic) {
			set_stacked_border(value.get_bool());
		} else if (key == uris.ingen_selected) {
			if (value.get_bool() != selected()) {
				if (value.get_bool()) {
					_canvas.lock()->select_item(shared_from_this());
				} else {
					_canvas.lock()->unselect_item(shared_from_this());
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
		if (App::instance().signal())
			App::instance().engine()->set_property(_model->path(),
				   App::instance().uris().ingen_selected, b);
	}
}



} // namespace GUI
} // namespace Ingen
