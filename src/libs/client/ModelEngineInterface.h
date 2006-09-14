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

#ifndef MODELENGINEINTERFACE_H
#define MODELENGINEINTERFACE_H

#include <string>
#include <lo/lo.h>
#include "interface/EngineInterface.h"
#include "ObjectModel.h"
using std::string;

class Path;

/** \defgroup IngenClient Client Library */
namespace Ingen {
namespace Client {

class ObjectModel;
class NodeModel;
class PresetModel;
class PatchModel;


/** Model-based engine command interface.
 *
 * \ingroup IngenClient
 */
class ModelEngineInterface : public virtual Shared::EngineInterface
{
public:
	virtual ~ModelEngineInterface() {}
	
	virtual void create_patch_from_model(const PatchModel* pm);

	virtual void create_node_with_data(const string&      plugin_uri,
	                                   const Path&        path,
	                                   bool               is_polyphonicc,
	                                   const MetadataMap& initial_data);

	virtual void set_metadata_map(const Path& subject, const MetadataMap& data);
	virtual void set_preset(const Path& patch_path, const PresetModel* pm);

protected:
	ModelEngineInterface() {}
};

} // namespace Client
} // namespace Ingen

#endif // MODELENGINEINTERFACE_H
