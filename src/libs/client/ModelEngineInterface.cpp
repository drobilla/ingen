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

#include "ModelEngineInterface.h"
#include "PatchModel.h"
#include "PresetModel.h"

namespace Ingen {
namespace Client {


/** Load a node.
 */
void
ModelEngineInterface::create_node_from_model(const NodeModel* nm)
{
	// Load by URI
	if (nm->plugin()->uri().length() > 0) {
		create_node(nm->path().c_str(),
		            nm->plugin()->type_string(),
	    	        nm->plugin()->uri().c_str(),
		    		nm->polyphonic());
	
	// Load by libname, label
	} else {
		//assert(nm->plugin()->lib_name().length() > 0);
		assert(nm->plugin()->plug_label().length() > 0);

		create_node(nm->path(), 
		            nm->plugin()->type_string(),
		            nm->plugin()->lib_name().c_str(),
		            nm->plugin()->plug_label().c_str(),
				    nm->polyphonic());
	}

	set_all_metadata(nm);
}


/** Create a patch.
 */
void
ModelEngineInterface::create_patch_from_model(const PatchModel* pm)
{
	create_patch(pm->path().c_str(), pm->poly());
	set_all_metadata(pm);
}


/** Set all pieces of metadata in a model.
 */
void
ModelEngineInterface::set_all_metadata(const ObjectModel* m)
{
	for (MetadataMap::const_iterator i = m->metadata().begin(); i != m->metadata().end(); ++i)
		set_metadata(m->path(), i->first, i->second);
}


/** Set a preset by setting all relevant controls for a patch.
 */
void
ModelEngineInterface::set_preset(const Path& patch_path, const PresetModel* const pm)
{
	for (list<ControlModel>::const_iterator i = pm->controls().begin(); i != pm->controls().end(); ++i) {
		set_port_value_queued((*i).port_path(), (*i).value());
	}
}


} // namespace Client
} // namespace Ingen
