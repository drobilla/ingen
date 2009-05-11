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

#include <cassert>
#include <iostream>
#include "PatchPortModule.hpp"
#include "interface/EngineInterface.hpp"
#include "client/PatchModel.hpp"
#include "client/NodeModel.hpp"
#include "App.hpp"
#include "PatchCanvas.hpp"
#include "Port.hpp"
#include "GladeFactory.hpp"
#include "RenameWindow.hpp"
#include "PatchWindow.hpp"
#include "WindowFactory.hpp"
#include "PortMenu.hpp"

namespace Ingen {
namespace GUI {


PatchPortModule::PatchPortModule(boost::shared_ptr<PatchCanvas> canvas, SharedPtr<PortModel> model)
	: FlowCanvas::Module(canvas, model->path().name(), 0, 0, false) // FIXME: coords?
	, _model(model)
	, _human_name_visible(false)
{
	assert(canvas);
	assert(model);

	assert(PtrCast<PatchModel>(model->parent()));

	set_stacked_border(model->polyphonic());

	model->signal_variable.connect(sigc::mem_fun(this, &PatchPortModule::set_variable));
	model->signal_property.connect(sigc::mem_fun(this, &PatchPortModule::set_property));
}


boost::shared_ptr<PatchPortModule>
PatchPortModule::create(boost::shared_ptr<PatchCanvas> canvas, SharedPtr<PortModel> model, bool human)
{
	boost::shared_ptr<PatchPortModule> ret(new PatchPortModule(canvas, model));
	boost::shared_ptr<Port> port(new Port(ret, model, model->symbol(), true));

	ret->add_port(port);
	ret->set_port(port);
	ret->set_menu(port->menu());
	
	for (GraphObject::Properties::const_iterator m = model->variables().begin();
			m != model->variables().end(); ++m)
		ret->set_variable(m->first, m->second);
	
	for (GraphObject::Properties::const_iterator m = model->properties().begin();
			m != model->properties().end(); ++m)
		ret->set_property(m->first, m->second);

	if (human)
		ret->show_human_names(human);
	else
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
	
	const Atom& existing_x = _model->get_property("ingenuity:canvas-x");
	const Atom& existing_y = _model->get_property("ingenuity:canvas-y");
	
	if (existing_x.type() != Atom::FLOAT || existing_y.type() != Atom::FLOAT
			|| existing_x.get_float() != x || existing_y.get_float() != y) {
		App::instance().engine()->set_property(_model->path(), "ingenuity:canvas-x", Atom(x));
		App::instance().engine()->set_property(_model->path(), "ingenuity:canvas-y", Atom(y));
	}
}


void
PatchPortModule::show_human_names(bool b)
{
	using namespace std;
	_human_name_visible = b;
	const Atom& name = _model->get_property("lv2:name");
	if (b && name.is_valid())
		set_name(name.get_string());
	else
		set_name(_model->symbol());
	
	resize();
}


void
PatchPortModule::set_name(const std::string& n)
{
	_port->set_name(n);
	Module::resize();
}


void
PatchPortModule::set_variable(const string& key, const Atom& value)
{
	if (key == "ingen:polyphonic" && value.type() == Atom::BOOL) {
		set_stacked_border(value.get_bool());
	} else if (key == "ingen:selected" && value.type() == Atom::BOOL) {
		if (value.get_bool() != selected()) {
			if (value.get_bool())
				_canvas.lock()->select_item(shared_from_this());
			else
				_canvas.lock()->unselect_item(shared_from_this());
		}
	}
}


void
PatchPortModule::set_property(const string& key, const Atom& value)
{
	if (key == "ingenuity:canvas-x" && value.type() == Atom::FLOAT) {
		move_to(value.get_float(), property_y());
	} else if (key == "ingenuity:canvas-y" && value.type() == Atom::FLOAT) {
		move_to(property_x(), value.get_float());
	} else if (key == "lv2:name" && value.type() == Atom::STRING && _human_name_visible) {
		set_name(value.get_string());
	} else if (key == "lv2:symbol" && value.type() == Atom::STRING && !_human_name_visible) {
		set_name(value.get_string());
	}
}


void
PatchPortModule::set_selected(bool b)
{
	if (b != selected()) {
		Module::set_selected(b);
		if (App::instance().signal())
			App::instance().engine()->set_variable(_model->path(), "ingen:selected", b);
	}
}



} // namespace GUI
} // namespace Ingen
