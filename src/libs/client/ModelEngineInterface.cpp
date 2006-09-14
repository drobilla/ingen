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
ModelEngineInterface::create_node_with_data(const string&      plugin_uri,
                                            const Path&        path,
                                            bool               is_polyphonic,
                                            const MetadataMap& initial_data)
{
	// Load by URI
	if (plugin_uri.length() > 0) {
		create_node(path, plugin_uri, is_polyphonic);
	
	// Load by libname, label
	} else {
		cerr << "FIXME: non-uri" << endl;
		#if 0
		//assert(nm->plugin()->lib_name().length() > 0);
		assert(nm->plugin()->plug_label().length() > 0);

		create_node(nm->path(), 
		            nm->plugin()->type_string(),
		            nm->plugin()->lib_name().c_str(),
		            nm->plugin()->plug_label().c_str(),
				    nm->polyphonic());
		#endif
	}

	set_metadata_map(path, initial_data);
}


/** Create a patch.
 */
void
ModelEngineInterface::create_patch_with_data(const Path& path, size_t poly, const MetadataMap& data)
{
	create_patch(path, poly);
	set_metadata_map(path, data);
}


/** Set all pieces of metadata in a map.
 */
void
ModelEngineInterface::set_metadata_map(const Path& subject, const MetadataMap& data)
{
	for (MetadataMap::const_iterator i = data.begin(); i != data.end(); ++i)
		set_metadata(subject, i->first, i->second);
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
