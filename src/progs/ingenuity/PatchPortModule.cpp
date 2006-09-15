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

#include "PatchPortModule.h"
#include <cassert>
#include "App.h"
#include "ModelEngineInterface.h"
#include "PatchCanvas.h"
#include "PatchModel.h"
#include "NodeModel.h"
#include "Port.h"
#include "GladeFactory.h"
#include "RenameWindow.h"
#include "PatchWindow.h"

namespace Ingenuity {


PatchPortModule::PatchPortModule(PatchCanvas* canvas, CountedPtr<PortModel> port)
: LibFlowCanvas::Module(canvas, "", 0, 0), // FIXME: coords?
  m_port(port),
  m_patch_port(NULL)
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

		m_patch_port = new Port(this, port, true);
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

	port->metadata_update_sig.connect(sigc::mem_fun(this, &PatchPortModule::metadata_update));
}


void
PatchPortModule::store_location()
{	
	const float x = static_cast<float>(property_x());
	const float y = static_cast<float>(property_y());
	
	const Atom& existing_x = m_port->get_metadata("module-x");
	const Atom& existing_y = m_port->get_metadata("module-y");
	
	if (existing_x.type() != Atom::FLOAT || existing_y.type() != Atom::FLOAT
			|| existing_x.get_float() != x || existing_y.get_float() != y) {
		App::instance().engine()->set_metadata(m_port->path(), "module-x", Atom(x));
		App::instance().engine()->set_metadata(m_port->path(), "module-y", Atom(y));
	}
}


void
PatchPortModule::metadata_update(const string& key, const Atom& value)
{
	if (key == "module-x" && value.type() == Atom::FLOAT)
		move_to(value.get_float(), property_y());
	else if (key == "module-y" && value.type() == Atom::FLOAT)
		move_to(property_x(), value.get_float());
}


} // namespace Ingenuity
