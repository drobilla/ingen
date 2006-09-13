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

#include "ModelClientInterface.h"
#include "PatchModel.h"
#include "ConnectionModel.h"
#include "PresetModel.h"
#include "NodeModel.h"
#include "PluginModel.h"

namespace Ingen {
namespace Client {


void
ModelClientInterface::new_plugin_model(CountedPtr<PluginModel> pi)
{
}


void
ModelClientInterface::new_patch_model(CountedPtr<PatchModel> pm)
{
}


void
ModelClientInterface::new_node_model(CountedPtr<NodeModel> nm)
{
}


void
ModelClientInterface::new_port_model(CountedPtr<PortModel> port_info)
{
}


void
ModelClientInterface::connection_model(CountedPtr<ConnectionModel> cm)
{
}



/* Implementations of ClientInterface functions to drive
 * the above functions:
 */



void
ModelClientInterface::new_plugin(string type,
           string uri,
           string name)
{
	CountedPtr<PluginModel> plugin(new PluginModel(type, uri));
	plugin->name(name);
	new_plugin_model(plugin);
}



void
ModelClientInterface::new_patch(string path, uint32_t poly)
{
	CountedPtr<PatchModel> pm(new PatchModel(path, poly));
	//PluginModel* pi = new PluginModel(PluginModel::Patch);
	//pm->plugin(pi);
	new_patch_model(pm);
}



void
ModelClientInterface::new_node(string   plugin_type,
                               string   plugin_uri,
                               string   node_path,
                               bool     is_polyphonic,
                               uint32_t num_ports)
{
	cerr << "FIXME: NEW NODE\n";
	
	CountedPtr<PluginModel> plugin(new PluginModel(plugin_type, plugin_uri));
	
	CountedPtr<NodeModel> nm(new NodeModel(plugin, node_path, is_polyphonic));

	new_node_model(nm);
}



void
ModelClientInterface::new_port(string path,
                               string type,
                               bool          is_output)
{
	PortModel::Type ptype = PortModel::CONTROL;
	if (type != "AUDIO") ptype = PortModel::AUDIO;
	else if (type != "CONTROL") ptype = PortModel::CONTROL;
	else if (type != "MIDI") ptype = PortModel::MIDI;
	else cerr << "[ModelClientInterface] WARNING:  Unknown port type received (" << type << ")" << endl;

	PortModel::Direction pdir = is_output ? PortModel::OUTPUT : PortModel::INPUT;
	
	CountedPtr<PortModel> port_model(new PortModel(path, ptype, pdir));
	new_port_model(port_model);
}



void
ModelClientInterface::connection(string src_port_path,
                                 string dst_port_path)
{
	connection_model(CountedPtr<ConnectionModel>(new ConnectionModel(src_port_path, dst_port_path)));
}




} // namespace Client
} // namespace Ingen
