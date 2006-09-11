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

#include "ControllerFactory.h"
#include "NodeModel.h"
#include "PatchModel.h"
#include "PortModel.h"
#include "NodeController.h"
#include "PatchController.h"
#include "PortController.h"
#include "DSSIController.h"

namespace Ingenuity {


CountedPtr<GtkObjectController>
ControllerFactory::get_controller(CountedPtr<ObjectModel> model)
{
	CountedPtr<PatchModel> patch_m = PtrCast<PatchModel>(model);
	if (patch_m && patch_m->controller()) {
		assert(dynamic_cast<PatchController*>(patch_m->controller().get()));
		return PtrCast<GtkObjectController>(patch_m->controller());
	} else if (patch_m) {
		CountedPtr<PatchController> pc(new PatchController(patch_m));
		patch_m->set_controller(pc);
		return pc;
	}

	CountedPtr<NodeModel> node_m = PtrCast<NodeModel>(model);
	if (node_m && node_m->controller()) {
		assert(dynamic_cast<NodeController*>(node_m->controller().get()));
		return PtrCast<GtkObjectController>(node_m->controller());
	} else if (node_m) {
		if (node_m->plugin()->type() == PluginModel::DSSI) {
			CountedPtr<NodeController> nc(new DSSIController(node_m));
			node_m->set_controller(nc);
			return nc;
		} else {
			CountedPtr<NodeController> nc(new NodeController(node_m));
			node_m->set_controller(nc);
			return nc;
		}
	}
	
	CountedPtr<PortModel> port_m = PtrCast<PortModel>(model);
	if (port_m && port_m->controller()) {
		assert(dynamic_cast<PortController*>(port_m->controller().get()));
		return PtrCast<GtkObjectController>(port_m->controller());
	} else if (port_m) {
		CountedPtr<PortController> pc(new PortController(port_m));
		port_m->set_controller(pc);
		return pc;
	}

	return CountedPtr<GtkObjectController>();
}


} // namespace Ingenuity
