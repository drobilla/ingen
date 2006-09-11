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

#ifndef DEMOLITIONMODEL_H
#define DEMOLITIONMODEL_H

#include <vector>
#include "PatchModel.h"
#include "util/Path.h"
using std::vector;

using namespace Ingen::Client;

class DemolitionModel {
public:
	~DemolitionModel();

	PatchModel*  random_patch();
	NodeModel*   random_node();
	NodeModel*   random_node_in_patch(PatchModel* patch);
	PortModel*   random_port();
	PortModel*   random_port_in_node(NodeModel* node);
	PluginModel* random_plugin();

	PatchModel* patch(const Path& path);
	NodeModel*  node(const Path& path);

	void add_patch(PatchModel* pm) { m_patches.push_back(pm); }
	void add_node(CountedPtr<NodeModel> nm);
	void add_port(PortModel* pm);
	void remove_object(const Path& path);
	void add_connection(ConnectionModel* cm);
	void remove_connection(const Path& src_port_path, const Path& dst_port_path);
	void add_plugin(PluginModel* pi) { m_plugins.push_back(pi); }

	void object_renamed(const Path& old_path, const Path& new_path);

	int num_patches() { return m_patches.size(); }

private:
	vector<PluginModel*> m_plugins;
	vector<PatchModel*>  m_patches;
};


#endif // DEMOLITIONMODEL_H
