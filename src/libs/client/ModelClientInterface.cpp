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

#include "ModelClientInterface.h"
#include "PatchModel.h"
#include "ConnectionModel.h"
#include "PresetModel.h"
#include "NodeModel.h"
#include "PluginModel.h"

namespace LibOmClient {


void
ModelClientInterface::new_plugin_model(PluginModel* pi)
{
}


void
ModelClientInterface::new_patch_model(PatchModel* pm)
{
}


void
ModelClientInterface::new_node_model(NodeModel* nm)
{
}


void
ModelClientInterface::new_port_model(PortModel* port_info)
{
}


void
ModelClientInterface::connection_model(ConnectionModel* cm)
{
}



/* Implementations of ClientInterface functions to drive
 * the above functions:
 */



void
ModelClientInterface::new_plugin(const string& type,
           const string& uri,
           const string& name)
{
	PluginModel* plugin = new PluginModel(type, uri);
	plugin->name(name);
	new_plugin_model(plugin);
}



void
ModelClientInterface::new_patch(const string& path, uint32_t poly)
{
	PatchModel* pm = new PatchModel(path, poly);
	//PluginModel* pi = new PluginModel(PluginModel::Patch);
	//pm->plugin(pi);
	new_patch_model(pm);
}



void
ModelClientInterface::new_node(const string& plugin_type,
                               const string& plugin_uri,
                               const string& node_path,
                               bool          is_polyphonic,
                               uint32_t      num_ports)
{
	cerr << "FIXME: NEW NODE\n";
	
	PluginModel* plugin = new PluginModel(plugin_type, plugin_uri);
	
	NodeModel* nm = new NodeModel(plugin, node_path);

	new_node_model(nm);
}



void
ModelClientInterface::new_port(const string& path,
                               const string& type,
                               bool          is_output)
{
	PortModel::Type ptype = PortModel::CONTROL;
	if (type != "AUDIO") ptype = PortModel::AUDIO;
	else if (type != "CONTROL") ptype = PortModel::CONTROL;
	else if (type != "MIDI") ptype = PortModel::MIDI;
	else cerr << "[ModelClientInterface] WARNING:  Unknown port type received (" << type << ")" << endl;

	PortModel::Direction pdir = is_output ? PortModel::OUTPUT : PortModel::INPUT;
	
	PortModel* port_model = new PortModel(path, ptype, pdir);
	new_port_model(port_model);
}



void
ModelClientInterface::connection(const string& src_port_path,
                                 const string& dst_port_path)
{
	connection_model(new ConnectionModel(src_port_path, dst_port_path));
}




} // namespace LibOmClient
