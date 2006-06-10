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

#include "OmModule.h"
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


OmModule::OmModule(OmFlowCanvas* canvas, NodeController* node)
: LibFlowCanvas::Module(canvas, node->node_model()->name(),
                        node->node_model()->x(), node->node_model()->y()),
  m_node(node)
{
	assert(m_node != NULL);

	/*if (node_model()->polyphonic() && node_model()->parent() != NULL
			&& node_model()->parent_patch()->poly() > 1) {
		border_width(2.0);
	}*/
	if (node->node_model()->polyphonic()) {
		border_width(2.0);
	}
}


void
OmModule::show_control_window()
{
	node()->show_control_window();
}


void
OmModule::store_location()
{
	if (m_node->node_model()->x() == 0 || m_node->node_model()->y() == 0)
		return;

	char temp_buf[16];
	
	m_node->node_model()->x(property_x());
	snprintf(temp_buf, 16, "%f", m_node->node_model()->x());
	m_node->node_model()->set_metadata("module-x", temp_buf); // just in case?
	Controller::instance().set_metadata(m_node->node_model()->path(), "module-x", temp_buf);
	
	m_node->node_model()->y(property_y());
	snprintf(temp_buf, 16, "%f", m_node->node_model()->y());
	m_node->node_model()->set_metadata("module-y", temp_buf); // just in case?
	Controller::instance().set_metadata(m_node->node_model()->path(), "module-y", temp_buf);
}


void
OmModule::move_to(double x, double y)
{
	Module::move_to(x, y);
	m_node->node_model()->x(x);
	m_node->node_model()->y(y);
	//store_location();
}

} // namespace OmGtk
