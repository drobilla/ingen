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

#ifndef CONSOLECLIENTHOOKS_H
#define CONSOLECLIENTHOOKS_H

#include "ClientHooks.h"

#include <string>
using std::string;

namespace LibOmClient {

class PatchModel;
class NodeModel;
class ConnectionModel;
class PortModel;
class MetadataModel;
class PluginModel;

class ConsoleClientHooks : public ClientHooks
{
public:
	ConsoleClientHooks()
	~ConsoleClientHooks();
	
	void error(const string& msg);
	void new_node(NodeModel* nm);
	
	void engine_enabled() {}
	void engine_disabled() {}
	void new_patch(PatchModel* pm) {}
	void new_port(PortModel* port_info) {}
	void port_removal(const string& path) {}
	void patch_destruction(const string& path) {}
	void patch_enabled(const string& path) {}
	void patch_disabled(const string& path) {}
	void node_removal(const string& path) {}
	void object_renamed(const string& old_path, const string& new_path) {}
	void connection(ConnectionModel* cm) {}
	void disconnection(const string& src_port_path, const string& dst_port_path) {}
	void metadata_update(MetadataModel* mm) {}
	void control_change(const string& port_path, float value) {}
	void new_plugin(PluginModel* pi) {}
	void program_add(const string& path, int bank, int program, const string& name) {}
	void program_remove(const string& path, int bank, int program) {}
};


} // namespace LibOmClient


#endif // CONSOLECLIENTHOOKS_H
