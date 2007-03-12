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


PatchPortModule::PatchPortModule(boost::shared_ptr<PatchCanvas> canvas, SharedPtr<PortModel> port)
: LibFlowCanvas::Module(canvas, port->path().name(), 0, 0, false), // FIXME: coords?
  _port(port)
{
	/*if (port_model()->polyphonic() && port_model()->parent() != NULL
			&& port_model()->parent_patch()->poly() > 1) {
		border_width(2.0);
	}*/
	
	assert(canvas);
	assert(port);

	assert(PtrCast<PatchModel>(port->parent()));

	/*resize();

	const Atom& x_atom = port->get_metadata("ingenuity:canvas-x");
	const Atom& y_atom = port->get_metadata("ingenuity:canvas-y");

	if (x_atom && y_atom && x_atom.type() == Atom::FLOAT && y_atom.type() == Atom::FLOAT) {
		move_to(x_atom.get_float(), y_atom.get_float());
	} else {
		double default_x;
		double default_y;
		canvas->get_new_module_location(default_x, default_y);
		move_to(default_x, default_y);
	}*/

	port->metadata_update_sig.connect(sigc::mem_fun(this, &PatchPortModule::metadata_update));
}


boost::shared_ptr<PatchPortModule>
PatchPortModule::create(boost::shared_ptr<PatchCanvas> canvas, SharedPtr<PortModel> port)
{
	boost::shared_ptr<PatchPortModule> ret = boost::shared_ptr<PatchPortModule>(
		new PatchPortModule(canvas, port));
	assert(ret);

	ret->_patch_port = boost::shared_ptr<Port>(new Port(ret, port, true, true));
	ret->add_port(ret->_patch_port);
	
	for (MetadataMap::const_iterator m = port->metadata().begin(); m != port->metadata().end(); ++m)
		ret->metadata_update(m->first, m->second);
	
	ret->resize();

	return ret;
}


void
PatchPortModule::store_location()
{	
	const float x = static_cast<float>(property_x());
	const float y = static_cast<float>(property_y());
	
	const Atom& existing_x = _port->get_metadata("ingenuity:canvas-x");
	const Atom& existing_y = _port->get_metadata("ingenuity:canvas-y");
	
	if (existing_x.type() != Atom::FLOAT || existing_y.type() != Atom::FLOAT
			|| existing_x.get_float() != x || existing_y.get_float() != y) {
		App::instance().engine()->set_metadata(_port->path(), "ingenuity:canvas-x", Atom(x));
		App::instance().engine()->set_metadata(_port->path(), "ingenuity:canvas-y", Atom(y));
	}
}


void
PatchPortModule::metadata_update(const string& key, const Atom& value)
{
	if (key == "ingenuity:canvas-x" && value.type() == Atom::FLOAT)
		move_to(value.get_float(), property_y());
	else if (key == "ingenuity:canvas-y" && value.type() == Atom::FLOAT)
		move_to(property_x(), value.get_float());
}


} // namespace Ingenuity
