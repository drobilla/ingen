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

#ifndef DEMOLITIONCLIENTHOOKS_H
#define DEMOLITIONCLIENTHOOKS_H

#include <cstdlib>
#include <string>
#include "ModelClientInterface.h"
#include "DemolitionModel.h"
#include "PluginModel.h"
using std::string;

using namespace Ingen::Client;


/** ModelClientInterface implementation for the Gtk client.
 *
 * These are the hooks into the GUI for the Comm class.
 *
 * \ingroup OmGtk
 */
class DemolitionClientInterface : public ModelClientInterface
{
public:
	DemolitionClientInterface(DemolitionModel* model);
	virtual ~DemolitionClientInterface();
	
	void bundle_begin() {}
	void bundle_end()   {}
	
	void transfer_begin() {}
	void transfer_end()   {}

	void num_plugins(uint32_t num) {}

	void response(int32_t id, bool success, string msg) {}

	// OSC thread functions
	void error(string msg);
	
	void new_plugin(string type,
	                string uri,
	                string name) {}
	void new_patch_model(CountedPtr<PatchModel> pm);
	void new_port_model(CountedPtr<PortModel> port_model);
	void object_destroyed(string path);
	void patch_enabled(string path);
	void patch_disabled(string path);
	void patch_cleared(string path) { throw; }
	void new_node_model(CountedPtr<NodeModel> nm);
	void object_renamed(string old_path, string new_path);
	void connection_model(CountedPtr<ConnectionModel> cm);
	void disconnection(string src_port_path, string dst_port_path);
	void metadata_update(string path, string key, string value) {}
	void control_change(string port_path, float value);
	void new_plugin_model(CountedPtr<PluginModel> pi);
	void program_add(string path, uint32_t bank, uint32_t program, string name) {};
	void program_remove(string path, uint32_t bank, uint32_t program) {};

private:
	DemolitionModel* m_model;
};


#endif // DEMOLITIONCLIENTHOOKS_H
