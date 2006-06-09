/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "PortController.h"
#include "OmModule.h"
#include "PortModel.h"
#include "ControlPanel.h"
#include "OmPort.h"
#include "Store.h"

namespace OmGtk {


PortController::PortController(PortModel* model)
: GtkObjectController(model),
  m_port(NULL),
  m_control_panel(NULL)
{
	assert(model != NULL);
	assert(model->parent() != NULL);
	assert(model->controller() == NULL);

	model->set_controller(this);
}


void
PortController::add_to_store()
{
	Store::instance().add_object(this);
}


void
PortController::remove_from_store()
{
	Store::instance().remove_object(this);
}


void
PortController::destroy()
{
	assert(m_model->parent() != NULL);
	NodeController* parent = (NodeController*)m_model->parent()->controller();
	assert(parent != NULL);

	if (m_control_panel != NULL)
		m_control_panel->remove_port(path());

	parent->remove_port(path(), false);
}


void
PortController::metadata_update(const string& key, const string& value)
{
	// FIXME: double lookups
	
	//cerr << path() << ": " << key << " = " << value << endl;

	if (key == "user-min") {
		port_model()->user_min(atof(value.c_str()));
		if (m_control_panel != NULL)
			m_control_panel->set_range_min(m_model->path(), atof(value.c_str()));
	} else if (key == "user-max") {
		port_model()->user_max(atof(value.c_str()));
		if (m_control_panel != NULL)
			m_control_panel->set_range_max(m_model->path(), atof(value.c_str()));
	}

	GtkObjectController::metadata_update(key, value);
}


void
PortController::control_change(float value)
{
	// FIXME: double lookups
	
	port_model()->value(value);
	
	if (m_control_panel != NULL)
		m_control_panel->set_control(port_model()->path(), value);
}


/** "Register" a control panel that is monitoring this port.
 *
 * The OmPort will handle notifying the ControlPanel when state
 * changes occur, etc.
 */
void
PortController::set_control_panel(ControlPanel* cp)
{
	assert(m_control_panel == NULL);
	m_control_panel = cp;
}


void
PortController::set_path(const Path& new_path)
{
	// Change port name on module, if view exists
	if (m_port != NULL)
		m_port->set_name(new_path.name());

	if (m_control_panel != NULL)
		m_control_panel->rename_port(m_model->path(), new_path);

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

