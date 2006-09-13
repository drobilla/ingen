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

#include "OmPortModule.h"
#include <cassert>
#include "App.h"
#include "ModelEngineInterface.h"
#include "OmFlowCanvas.h"
#include "PatchModel.h"
#include "NodeModel.h"
#include "OmPort.h"
#include "GladeFactory.h"
#include "RenameWindow.h"
#include "PatchWindow.h"
#include "OmPatchPort.h"

namespace Ingenuity {


OmPortModule::OmPortModule(OmFlowCanvas* canvas, CountedPtr<PortModel> port)
: LibFlowCanvas::Module(canvas, "", 0, 0), // FIXME: coords?
  m_port(port)
{
	/*if (port_model()->polyphonic() && port_model()->parent() != NULL
			&& port_model()->parent_patch()->poly() > 1) {
		border_width(2.0);
	}*/
	
	assert(canvas);
	assert(port);

	if (PtrCast<PatchModel>(port->parent())) {
		if (m_patch_port)
			delete m_patch_port;

		m_patch_port = new OmPatchPort(this, port);
	}
	
	resize();

	const Atom& x_atom = port->get_metadata("module-x");
	const Atom& y_atom = port->get_metadata("module-y");

	if (x_atom && y_atom && x_atom.type() == Atom::FLOAT && y_atom.type() == Atom::FLOAT) {
		move_to(x_atom.get_float(), y_atom.get_float());
	} else {
		double default_x;
		double default_y;
		canvas->get_new_module_location(default_x, default_y);
		move_to(default_x, default_y);
	}
}


void
OmPortModule::store_location()
{
	char temp_buf[16];
	
	//m_port->x(property_x());
	snprintf(temp_buf, 16, "%f", property_x().get_value());
	//m_port->set_metadata("module-x", temp_buf); // just in case?
	App::instance().engine()->set_metadata(m_port->path(), "module-x", temp_buf);
	
	//m_port->y(property_y());
	snprintf(temp_buf, 16, "%f", property_y().get_value());
	//m_port->set_metadata("module-y", temp_buf); // just in case?
	App::instance().engine()->set_metadata(m_port->path(), "module-y", temp_buf);
}


void
OmPortModule::move_to(double x, double y)
{
	Module::move_to(x, y);
	//m_port->x(x);
	//m_port->y(y);
	//store_location();
}

} // namespace Ingenuity
