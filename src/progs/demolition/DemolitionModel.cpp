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

#include "DemolitionModel.h"
#include "util/Path.h"
#include "PluginModel.h"
#include <stdlib.h>
#include <iostream>
using std::cout; using std::cerr; using std::endl;


DemolitionModel::~DemolitionModel()
{
	for (vector<PluginModel*>::iterator i = m_plugins.begin(); i != m_plugins.end(); ++i)
		delete *i;

	for (vector<PatchModel*>::iterator i = m_patches.begin(); i != m_patches.end(); ++i)
		delete *i;
}


PatchModel*
DemolitionModel::random_patch()
{
	if (m_patches.size() == 0)
		return NULL;
	else
		return m_patches.at(rand() % m_patches.size());
}


NodeModel*
DemolitionModel::random_node()
{
	int max_attempts = m_patches.size()*4;
	int attempts = 0;

	while (attempts++ < max_attempts) {
		PatchModel* pm = random_patch();
	
		if (pm == NULL) {
			return NULL;
		} else {
			int size = pm->nodes().size();
			if (size == 0)
				continue;
			int index = rand() % size;
			NodeModelMap::const_iterator i = pm->nodes().begin();
			for (int j=0; j < index; ++j) {
				++i;
				if (i == pm->nodes().end())
					return NULL;
			}
			return (*i).second.get();
		}
	}
	//cout << "***************************** Not returning node *********" << endl;
	return NULL;
}


NodeModel*
DemolitionModel::random_node_in_patch(PatchModel* pm)
{
	if (pm == NULL)
		return NULL;
	
	int size = pm->nodes().size();
	if (size == 0)
		return NULL;

	int index = rand() % size;
	NodeModelMap::const_iterator i = pm->nodes().begin();
	for (int j=0; j < index; ++j) {
		++i;
		if (i == pm->nodes().end())
			return NULL;
	}
	return (*i).second.get();
}


PortModel*
DemolitionModel::random_port()
{
	NodeModel* node = random_node();

	if (node == NULL) return NULL;

	PortModelList ports = node->ports();

	int num_ports = ports.size();
	int index = rand() % num_ports;
	int i = 0;
	
	for (PortModelList::iterator p = ports.begin(); p != ports.end(); ++p, ++i)
		if (i == index)
			return (*p).get();

	return NULL; // shouldn't happen
}


PortModel*
DemolitionModel::random_port_in_node(NodeModel* node)
{
	if (node == NULL)
		return NULL;

	PortModelList ports = node->ports();

	int num_ports = ports.size();
	int index = rand() % num_ports;
	int i = 0;
	
	for (PortModelList::iterator p = ports.begin(); p != ports.end(); ++p, ++i)
		if (i == index)
			return (*p).get();

	return NULL; // shouldn't happen
}


PluginModel*
DemolitionModel::random_plugin()
{
	if (m_plugins.size() == 0)
		return NULL;
	else
		return m_plugins.at(rand() % m_plugins.size());
}


PatchModel*
DemolitionModel::patch(const Path& path)
{
	for (vector<PatchModel*>::iterator i = m_patches.begin(); i != m_patches.end(); ++i) {
		if ((*i)->path() == path)
			return (*i);
	}
	return NULL;
}


NodeModel*
DemolitionModel::node(const Path& path)
{
	NodeModel* ret = NULL;
	
	for (vector<PatchModel*>::iterator i = m_patches.begin(); i != m_patches.end(); ++i) {
		ret = (*i)->get_node(path.name()).get();
		if (ret != NULL)
			break;
	}
	return ret;
}


void
DemolitionModel::remove_object(const Path& path)
{
	// Patch
	for (vector<PatchModel*>::iterator i = m_patches.begin(); i != m_patches.end(); ++i) {
		if ((*i)->path() == path) {
			delete (*i);
			m_patches.erase(i);
			break;
		}
	}

	// Node
	PatchModel* parent = patch(path.parent());
	if (parent != NULL)
		parent->remove_node(path.name());
}


void
DemolitionModel::add_node(NodeModel* nm)
{
	PatchModel* parent = patch(nm->path().parent());
	if (parent == NULL) {
		cout << "**** ERROR:  Parent for new Node does not exist." << endl;
	} else {
		parent->add_node(nm);
	}
}


void
DemolitionModel::add_port(PortModel* pm)
{
}


void
DemolitionModel::add_connection(ConnectionModel* cm)
{
}


void
DemolitionModel::remove_connection(const Path& src_port_path, const Path& dst_port_path)
{
}

void
DemolitionModel::object_renamed(const Path& old_path, const Path& new_path)
{
	/* FIXME: broken, does not rename children
	assert(OmPath::parent(old_path) == OmPath::parent(new_path));
	
	// Rename node
	NodeModel* nm = get_node(old_path);
	if (nm != NULL) {
		if (nm->parent() != NULL) {
			nm->parent()->remove_node(OmPath::name(old_path));
			nm->path(new_path);
			nm->parent()->add_node(OmPath::name(new_path));
		}
	}

	// Rename patch
	for (vector<PatchModel*>::iterator i = m_patches.begin(); i != m_patches.end(); ++i) {
		if ((*i)->path() == path) {
			delete (*i);
			m_patches.erase(i);
			return;
		}
	}
	*/
}

