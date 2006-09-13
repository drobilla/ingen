/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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

#include "OmModule.h"
#include <cassert>
#include "util/Atom.h"
#include "App.h"
#include "ModelEngineInterface.h"
#include "OmFlowCanvas.h"
#include "PatchModel.h"
#include "NodeModel.h"
#include "OmPort.h"
#include "GladeFactory.h"
#include "RenameWindow.h"
#include "PatchWindow.h"
#include "WindowFactory.h"

namespace Ingenuity {


OmModule::OmModule(OmFlowCanvas* canvas, CountedPtr<NodeModel> node)
: LibFlowCanvas::Module(canvas, node->path().name()),
  m_node(node),
  m_menu(node)
{
	assert(m_node);

	if (node->polyphonic()) {
		border_width(2.0);
	}

	create_all_ports();
	
	const Atom& x = node->get_metadata("module-x");
	const Atom& y = node->get_metadata("module-y");

	if (x.type() == Atom::FLOAT && y.type() == Atom::FLOAT) {
		move_to(x.get_float(), y.get_float());
	} else {
		double x, y;
		((OmFlowCanvas*)m_canvas)->get_new_module_location(x, y);
	}

	node->new_port_sig.connect(sigc::mem_fun(this, &OmModule::add_port));
	node->removed_port_sig.connect(sigc::mem_fun(this, &OmModule::remove_port));
	node->metadata_update_sig.connect(sigc::mem_fun(this, &OmModule::metadata_update));
}


void
OmModule::create_all_ports()
{
	for (PortModelList::const_iterator i = m_node->ports().begin();
			i != m_node->ports().end(); ++i) {
		add_port(*i);
	}

	resize();

	// FIXME
	//if (has_control_inputs())
	//	enable_controls_menuitem();
}


void
OmModule::add_port(CountedPtr<PortModel> port)
{
	new OmPort(this, port);
	resize();
}


void
OmModule::remove_port(CountedPtr<PortModel> port)
{
	LibFlowCanvas::Port* canvas_port = get_port(port->path().name());
	delete canvas_port;
}


void
OmModule::show_control_window()
{
	App::instance().window_factory()->present_controls(m_node);
}


void
OmModule::store_location()
{
	const float x = static_cast<float>(property_x());
	const float y = static_cast<float>(property_y());
	
	const Atom& existing_x = m_node->get_metadata("module-x");
	const Atom& existing_y = m_node->get_metadata("module-y");
	
	if (existing_x.type() != Atom::FLOAT || existing_y.type() != Atom::FLOAT
			|| existing_x.get_float() != x || existing_y.get_float() != y) {
		App::instance().engine()->set_metadata(m_node->path(), "module-x", Atom(x));
		App::instance().engine()->set_metadata(m_node->path(), "module-y", Atom(y));
	}

}


void
OmModule::move_to(double x, double y)
{
	Module::move_to(x, y);
	//store_location();
}


void
OmModule::on_right_click(GdkEventButton* event)
{
	m_menu.popup(event->button, event->time);
}


void
OmModule::metadata_update(const string& key, const Atom& value)
{
	if (key == "module-x" && value.type() == Atom::FLOAT)
		move_to(value.get_float(), property_y());
	else if (key == "module-y" && value.type() == Atom::FLOAT)
		move_to(property_x(), value.get_float());
}


} // namespace Ingenuity
