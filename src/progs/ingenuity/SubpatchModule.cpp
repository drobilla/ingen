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

#include "SubpatchModule.h"
#include <cassert>
#include <iostream>
#include "OmModule.h"
#include "NodeControlWindow.h"
#include "PatchModel.h"
#include "PatchWindow.h"
#include "OmFlowCanvas.h"
#include "PatchController.h"
#include "OmPort.h"
#include "Controller.h"
using std::cerr; using std::cout; using std::endl;

namespace OmGtk {


SubpatchModule::SubpatchModule(OmFlowCanvas* canvas, PatchController* patch)
: OmModule(canvas, patch),
  m_patch(patch)
{
	assert(canvas != NULL);
	assert(patch != NULL);
}


void
SubpatchModule::add_om_port(PortModel* pm, bool resize_to_fit)
{
	OmPort* port = new OmPort(this, pm);

	port->signal_event().connect(
		sigc::bind<Port*>(sigc::mem_fun(m_canvas, &OmFlowCanvas::port_event), port));
	
	Module::add_port(port, resize_to_fit);
}


void
SubpatchModule::on_double_click(GdkEventButton* event)
{
	assert(m_patch != NULL);

	// If window is visible
	if (m_patch->window() != NULL
			&& m_patch->window()->is_visible()) {
		m_patch->show_patch_window(); // just raise it
	// No window visible
	} else {
		if (event->state & GDK_SHIFT_MASK)
			m_patch->show_patch_window(); // open a new window
		else
			browse_to_patch();
	}
}



/** Browse to this patch in current (parent's) window. */
void
SubpatchModule::browse_to_patch()
{
	assert(m_patch->model()->parent());
	PatchController* pc = (PatchController*)m_patch->model()->parent()->controller();
	assert(pc != NULL);
	assert(pc->window() != NULL);
	
	assert(pc->window() != NULL);
	pc->window()->patch_controller(m_patch);
}



void
SubpatchModule::show_dialog()
{
	m_patch->show_control_window();
}


void
SubpatchModule::menu_remove()
{
	Controller::instance().destroy(m_patch->model()->path());
}

} // namespace OmGtk
