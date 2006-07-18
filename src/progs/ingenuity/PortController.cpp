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

#include "PortController.h"
#include "OmModule.h"
#include "PortModel.h"
#include "ControlPanel.h"
#include "OmPort.h"
#include "OmPatchPort.h"
#include "Store.h"

namespace OmGtk {


PortController::PortController(CountedPtr<PortModel> model)
: GtkObjectController(model),
  m_module(NULL),
  m_port(NULL)
  //m_control_panel(NULL)
{
	assert(model);
	assert(model->parent());
	assert(model->controller() == NULL);

	model->set_controller(this);
}


void
PortController::destroy()
{
	assert(m_model->parent());
	NodeController* parent = (NodeController*)m_model->parent()->controller();
	assert(parent != NULL);

	//if (m_control_panel != NULL)
	//	m_control_panel->remove_port(path());

	parent->remove_port(path(), false);
}


void
PortController::create_module(OmFlowCanvas* canvas, double x, double y)
{
	cerr << "Creating port module " << m_model->path() << endl;

	assert(canvas);
	assert(port_model());
	m_module = new OmPortModule(canvas, this, x, y);
	
	// FIXME: leak
	m_patch_port = new OmPatchPort(m_module, port_model());
	m_module->add_port(m_patch_port, false);

	m_module->move_to(x, y);
}


void
PortController::metadata_update(const string& key, const string& value)
{
	// FIXME: double lookups
	
	//cerr << path() << ": " << key << " = " << value << endl;

/* Panel now listens to model signals..

	if (key == "user-min") {
		port_model()->user_min(atof(value.c_str()));
		if (m_control_panel != NULL)
			m_control_panel->set_range_min(m_model->path(), atof(value.c_str()));
	} else if (key == "user-max") {
		port_model()->user_max(atof(value.c_str()));
		if (m_control_panel != NULL)
			m_control_panel->set_range_max(m_model->path(), atof(value.c_str()));
	}
*/
	cerr << "FIXME: PortController::metadata_update" << endl;

	if (m_module != NULL) {
		if (key == "module-x") {
			float x = atof(value.c_str());
			//if (x > 0 && x < m_canvas->width())
				m_module->move_to(x, m_module->property_y().get_value());
		} else if (key == "module-y") {
			float y = atof(value.c_str());
			//if (y > 0 && y < m_canvas->height())
				m_module->move_to(m_module->property_x().get_value(), y);
		}
	}

	GtkObjectController::metadata_update(key, value);
}


/** "Register" a control panel that is monitoring this port.
 *
 * The OmPort will handle notifying the ControlPanel when state
 * changes occur, etc.
 */
/*
void
PortController::set_control_panel(ControlPanel* cp)
{
	assert(m_control_panel == NULL);
	m_control_panel = cp;
}
*/

void
PortController::set_path(const Path& new_path)
{
	// Change port name on module, if view exists
	if (m_port != NULL)
		m_port->set_name(new_path.name());

	//if (m_control_panel != NULL)
	//	m_control_panel->rename_port(m_model->path(), new_path);

	m_model->set_path(new_path);
}


/** Create the visible port on a canvas module.
 *
 * Does not resize the module, caller's responsibility to do so if necessary.
 */
void
PortController::create_port(OmModule* module)
{
	assert(module != NULL);

	m_port = new OmPort(module, port_model());
	module->add_port(m_port, false);
}


} // namespace OmGtk

