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

#include "DSSIModule.h"
#include "DSSIController.h"

namespace OmGtk {


DSSIModule::DSSIModule(OmFlowCanvas* canvas, DSSIController* node)
: OmModule(canvas, static_cast<NodeController*>(node))
{
}


void
DSSIModule::on_double_click(GdkEventButton* ev)
{
	DSSIController* const dc = static_cast<DSSIController*>(m_node);
	if (!dc->attempt_to_show_gui())
		show_control_window();
}


} // namespace OmGtk
