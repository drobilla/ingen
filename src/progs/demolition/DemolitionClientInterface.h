/* This file is part of Om.  Copyright (C) 2005 Dave Robillard.
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
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#ifndef DEMOLITIONCLIENTHOOKS_H
#define DEMOLITIONCLIENTHOOKS_H

#include <cstdlib>
#include <string>
#include "ModelClientInterface.h"
#include "DemolitionModel.h"
#include "PluginModel.h"
using std::string;

using namespace LibOmClient;


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

	void num_plugins(size_t num) {}

	// OSC thread functions
	void error(const string& msg);
	
	void new_plugin(const string& type,
	                const string& uri,
	                const string& name) {}
	void new_patch_model(PatchModel* const pm);
	void new_port_model(PortModel* const port_model);
	void object_destroyed(const string& path);
	void patch_enabled(const string& path);
	void patch_disabled(const string& path);
	void patch_cleared(const string& path) { throw; }
	void new_node_model(NodeModel* const nm);
	void object_renamed(const string& old_path, const string& new_path);
	void connection_model(ConnectionModel* const cm);
	void disconnection(const string& src_port_path, const string& dst_port_path);
	void metadata_update(const string& path, const string& key, const string& value) {}
	void control_change(const string& port_path, float value);
	void new_plugin_model(PluginModel* const pi);
	void program_add(const string& path, uint32_t bank, uint32_t program, const string& name) {};
	void program_remove(const string& path, uint32_t bank, uint32_t program) {};

private:
	DemolitionModel* m_model;
};


#endif // DEMOLITIONCLIENTHOOKS_H
