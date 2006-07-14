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
using std::string;

/** \defgroup libomclient Client Library
 */

namespace LibOmClient {

class NodeModel;
class PresetModel;
class PatchModel;
class OSCListener;
class ModelClientInterface;


/** Model-based engine command interface.
 *
 * \ingroup libomclient
 */
class ModelEngineInterface
{
public:
	virtual ~ModelEngineInterface() {}
	
	virtual void create_patch_from_model(const PatchModel* pm) = 0;
	virtual void create_node_from_model(const NodeModel* nm) = 0;

	virtual void set_all_metadata(const NodeModel* nm) = 0;
	virtual void set_preset(const string& patch_path, const PresetModel* pm) = 0;

protected:
	ModelEngineInterface() {}
};

} // namespace LibOmClient

#endif // MODELENGINEINTERFACE_H
