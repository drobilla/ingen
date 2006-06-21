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
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "OmPortModule.h"
#include <cassert>
#include "Controller.h"
#include "OmFlowCanvas.h"
#include "PatchModel.h"
#include "NodeModel.h"
#include "OmPort.h"
#include "GladeFactory.h"
#include "RenameWindow.h"
#include "PatchController.h"
#include "PatchWindow.h"

namespace OmGtk {


OmPortModule::OmPortModule(OmFlowCanvas* canvas, PortController* port, double x, double y)
: LibFlowCanvas::Module(canvas, "", x, y),
  m_port(port)
{
	assert(m_port != NULL);

	/*if (port_model()->polyphonic() && port_model()->parent() != NULL
			&& port_model()->parent_patch()->poly() > 1) {
		border_width(2.0);
	}*/
}


void
OmPortModule::store_location()
{
	char temp_buf[16];
	
	//m_port->port_model()->x(property_x());
	snprintf(temp_buf, 16, "%f", property_x().get_value());
	//m_port->port_model()->set_metadata("module-x", temp_buf); // just in case?
	Controller::instance().set_metadata(m_port->port_model()->path(), "module-x", temp_buf);
	
	//m_port->port_model()->y(property_y());
	snprintf(temp_buf, 16, "%f", property_y().get_value());
	//m_port->port_model()->set_metadata("module-y", temp_buf); // just in case?
	Controller::instance().set_metadata(m_port->port_model()->path(), "module-y", temp_buf);
}


void
OmPortModule::move_to(double x, double y)
{
	Module::move_to(x, y);
	//m_port->port_model()->x(x);
	//m_port->port_model()->y(y);
	//store_location();
}

} // namespace OmGtk
