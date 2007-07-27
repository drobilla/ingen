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
#include "client/PatchModel.hpp"
#include "client/PortModel.hpp"
#include "client/ControlModel.hpp"
#include "Configuration.hpp"
#include "App.hpp"
#include "Port.hpp"
using std::cerr; using std::endl;

using namespace Ingen::Client;

namespace Ingen {
namespace GUI {


/** @param flip Make an input port appear as an output port, and vice versa.
 */
Port::Port(boost::shared_ptr<FlowCanvas::Module> module, SharedPtr<PortModel> pm, bool flip, bool destroyable)
: FlowCanvas::Port(module,
		pm->path().name(),
		flip ? (!pm->is_input()) : pm->is_input(),
		App::instance().configuration()->get_port_color(pm.get())),
  _port_model(pm)
{
	assert(module);
	assert(_port_model);

	if (destroyable)
		_menu.items().push_back(Gtk::Menu_Helpers::MenuElem("Destroy",
				sigc::mem_fun(this, &Port::on_menu_destroy)));

	control_changed(_port_model->value());

	_port_model->renamed_sig.connect(sigc::mem_fun(this, &Port::renamed));

	_port_model->control_change_sig.connect(sigc::mem_fun(this, &Port::control_changed));
}


void
Port::on_menu_destroy()
{
	App::instance().engine()->destroy(_port_model->path());
}


void
Port::renamed()
{
	set_name(_port_model->path().name());
}


void
Port::control_changed(float value)
{
	float min = 0.0f, max = 1.0f;
	boost::shared_ptr<NodeModel> parent = PtrCast<NodeModel>(_port_model->parent());
	if (parent)
		parent->port_value_range(_port_model->path().name(), min, max);
		
	/*cerr << "Control changed: " << value << endl;
	cerr << "Min: " << min << endl;
	cerr << "Max: " << max << endl;*/

	FlowCanvas::Port::set_control((value - min) / (max - min));
}


void
Port::set_control(float value)
{
	if (_port_model->is_control()) {
		//cerr << "Set control: " << value << endl;
		float min = 0.0f, max = 1.0f;
		boost::shared_ptr<NodeModel> parent = PtrCast<NodeModel>(_port_model->parent());
		if (parent)
			parent->port_value_range(_port_model->path().name(), min, max);

		App::instance().engine()->set_port_value(_port_model->path(),
				min + (value * (max-min)));

		FlowCanvas::Port::set_control(value);
	}
}


} // namespace GUI
} // namespace Ingen
