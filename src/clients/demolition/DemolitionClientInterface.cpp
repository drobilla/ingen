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

#include "DemolitionClientInterface.h"
#include "DemolitionModel.h"


DemolitionClientInterface::DemolitionClientInterface(DemolitionModel* model)
: m_model(model)
{
}


DemolitionClientInterface::~DemolitionClientInterface()
{
}


void
DemolitionClientInterface::error(const string& msg)
{
}


void
DemolitionClientInterface::new_patch_model(PatchModel* pm)
{
	m_model->add_patch(pm);
}


void
DemolitionClientInterface::new_port_model(PortModel* port_model)
{
	m_model->add_port(port_model);
}


void
DemolitionClientInterface::object_destroyed(const string& path)
{
	m_model->remove_object(path);
}

void
DemolitionClientInterface::patch_enabled(const string& path)
{
}


void
DemolitionClientInterface::patch_disabled(const string& path)
{
}


void
DemolitionClientInterface::new_node_model(NodeModel* nm)
{
	m_model->add_node(nm);
}


void
DemolitionClientInterface::object_renamed(const string& old_path, const string& new_path)
{
	m_model->object_renamed(old_path, new_path);
}


void
DemolitionClientInterface::connection_model(ConnectionModel* cm)
{
	m_model->add_connection(cm);
}


void
DemolitionClientInterface::disconnection(const string& src_port_path, const string& dst_port_path)
{
	m_model->remove_connection(src_port_path, dst_port_path);
}


void
DemolitionClientInterface::control_change(const string& port_path, float value)
{
}


void
DemolitionClientInterface::new_plugin_model(PluginModel* pi)
{
	m_model->add_plugin(pi);
}

