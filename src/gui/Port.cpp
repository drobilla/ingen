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
#include "interface/EngineInterface.hpp"
#include "flowcanvas/Module.hpp"
#include "client/PatchModel.hpp"
#include "client/PortModel.hpp"
#include "Configuration.hpp"
#include "App.hpp"
#include "Port.hpp"
#include "PortMenu.hpp"
#include "GladeFactory.hpp"

using namespace Ingen::Client;
using namespace std;

namespace Ingen {
namespace GUI {


/** @a flip Make an input port appear as an output port, and vice versa.
 */
Port::Port(
		boost::shared_ptr<FlowCanvas::Module> module,
		SharedPtr<PortModel>                  pm,
		const string&                         name,
		bool                                  flip)
	: FlowCanvas::Port(module, name,
			flip ? (!pm->is_input()) : pm->is_input(),
			App::instance().configuration()->get_port_color(pm.get()))
	, _port_model(pm)
	, _flipped(flip)
{
	assert(module);
	assert(pm);

	delete _menu;
	_menu = NULL;
	
	pm->signal_renamed.connect(sigc::mem_fun(this, &Port::renamed));

	if (pm->type().is_control()) {
		set_toggled(pm->is_toggle());
		show_control();
		
		float min = 0.0f, max = 1.0f;
		boost::shared_ptr<NodeModel> parent = PtrCast<NodeModel>(pm->parent());
		if (parent)
			parent->port_value_range(pm, min, max);

		set_control_min(min);
		set_control_max(max);

		pm->signal_variable.connect(sigc::mem_fun(this, &Port::variable_changed));
		pm->signal_value_changed.connect(sigc::mem_fun(this, &Port::value_changed));
	}
		
	pm->signal_activity.connect(sigc::mem_fun(this, &Port::activity));
	
	value_changed(pm->value());
}


Port::~Port()
{
	App::instance().activity_port_destroyed(this);
}


void
Port::create_menu()
{
	PortMenu* menu = NULL;
	Glib::RefPtr<Gnome::Glade::Xml> xml = GladeFactory::new_glade_reference();
	xml->get_widget_derived("object_menu", menu);
	menu->init(model(), _flipped);
	set_menu(menu);
}


void
Port::renamed()
{
	set_name(model()->path().name());
	module().lock()->resize();
}


void
Port::value_changed(const Atom& value)
{
	if (value.type() == Atom::FLOAT)
		FlowCanvas::Port::set_control(value.get_float());
	else
		cerr << "WARNING: Unknown port value type " << (unsigned)value.type() << endl;
}

	
void
Port::activity()
{
	App::instance().port_activity(this);
}


void
Port::set_control(float value, bool signal)
{
	if (signal) {
		if (model()->type() == DataType::CONTROL) {
			App::instance().engine()->set_port_value(model()->path(), Atom(value));
		} else if (model()->type() == DataType::EVENT) {
			App::instance().engine()->set_port_value(model()->path(),
					Atom("<http://example.org/ev#BangEvent>", 0, NULL));
		}
	}

	FlowCanvas::Port::set_control(value);
}


void
Port::variable_changed(const string& key, const Atom& value)
{
	if ( (key == "lv2:minimum") && value.type() == Atom::FLOAT)
		set_control_min(value.get_float());
	else if ( (key == "lv2:maximum") && value.type() == Atom::FLOAT)
		set_control_max(value.get_float());
	else if ( (key == "lv2:toggled") && value.type() == Atom::BOOL)
		set_toggled(value.get_bool());
}


} // namespace GUI
} // namespace Ingen
