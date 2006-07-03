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
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "PatchModel.h"
#include "NodeModel.h"
#include "ConnectionModel.h"
#include <cassert>
#include <iostream>

using std::cerr; using std::cout; using std::endl;

namespace LibOmClient {


void
PatchModel::set_path(const Path& new_path)
{
	// FIXME: haack
	if (new_path == "") {
		m_path = "";
		return;
	}

	NodeModel::set_path(new_path);
	for (NodeModelMap::iterator i = m_nodes.begin(); i != m_nodes.end(); ++i)
		(*i).second->set_path(m_path +"/"+ (*i).second->name());
	
#ifdef DEBUG
	// Be sure connection paths are updated and sane
	for (list<CountedPtr<ConnectionModel> >::iterator j = m_connections.begin();
			j != m_connections.end(); ++j) {
		assert((*j)->src_port_path().parent().parent() == new_path);
		assert((*j)->src_port_path().parent().parent() == new_path);
	}
#endif
}


CountedPtr<NodeModel>
PatchModel::get_node(const string& name)
{
	assert(name.find("/") == string::npos);
	NodeModelMap::iterator i = m_nodes.find(name);
	return ((i != m_nodes.end()) ? (*i).second : CountedPtr<NodeModel>(NULL));
}


void
PatchModel::add_node(CountedPtr<NodeModel> nm)
{
	assert(nm);
	assert(nm->name().find("/") == string::npos);
	assert(nm->parent().get() == this);
	assert(m_nodes.find(nm->name()) == m_nodes.end());

	m_nodes[nm->name()] = nm;

	new_node_sig.emit(nm);
}


void
PatchModel::remove_node(const string& name)
{
	assert(name.find("/") == string::npos);
	NodeModelMap::iterator i = m_nodes.find(name);
	if (i != m_nodes.end()) {
		//delete i->second;
		m_nodes.erase(i);
		removed_node_sig.emit(name);
		return;
	}
	
	cerr << "[PatchModel::remove_node] " << m_path << ": failed to find node " << name << endl;
}


void
PatchModel::clear()
{
	//for (list<CountedPtr<ConnectionModel> >::iterator j = m_connections.begin(); j != m_connections.end(); ++j)
	//	delete (*j);
	
	for (NodeModelMap::iterator i = m_nodes.begin(); i != m_nodes.end(); ++i) {
		(*i).second->clear();
		//delete (*i).second;
	}
	
	m_nodes.clear();
	m_connections.clear();

	NodeModel::clear();

	assert(m_nodes.empty());
	assert(m_connections.empty());
	assert(m_ports.empty());
}


/** Updated the map key of the given node.
 *
 * The NodeModel must already have it's new path set to @a new_path, or this will
 * abort with a fatal error.
 */
void
PatchModel::rename_node(const Path& old_path, const Path& new_path)
{
	assert(old_path.parent() == path());
	assert(new_path.parent() == path());
	
	NodeModelMap::iterator i = m_nodes.find(old_path.name());
	NodeModel* nm = NULL;
	
	if (i != m_nodes.end()) {
		nm = (*i).second.get();
		for (list<CountedPtr<ConnectionModel> >::iterator j = m_connections.begin(); j != m_connections.end(); ++j) {
			if ((*j)->src_port_path().parent() == old_path)
				(*j)->src_port_path(new_path.base_path() + (*j)->src_port_path().name());
			if ((*j)->dst_port_path().parent() == old_path)
				(*j)->dst_port_path(new_path.base_path() + (*j)->dst_port_path().name());
		}
		m_nodes.erase(i);
		assert(nm->path() == new_path);
		m_nodes[new_path.name()] = nm;
		return;
	}
	
	cerr << "[PatchModel::rename_node] " << m_path << ": failed to find node " << old_path << endl;
}


CountedPtr<ConnectionModel>
PatchModel::get_connection(const string& src_port_path, const string& dst_port_path)
{
	for (list<CountedPtr<ConnectionModel> >::iterator i = m_connections.begin(); i != m_connections.end(); ++i)
		if ((*i)->src_port_path() == src_port_path && (*i)->dst_port_path() == dst_port_path)
			return (*i);
	return NULL;
}


/** Add a connection to this patch.
 *
 * Ownership of @a cm is taken, it will be deleted along with this PatchModel.
 * If @a cm only contains paths (not pointers to the actual ports), the ports
 * will be found and set.  The ports referred to not existing as children of
 * this patch is a fatal error.
 */
void
PatchModel::add_connection(CountedPtr<ConnectionModel> cm)
{
	assert(cm);
	//assert(cm->src_port_path().parent().parent() == m_path);
	//assert(cm->dst_port_path().parent().parent() == m_path);
	assert(cm->patch_path() == path());
	
	cerr << "PatchModel::add_connection: " << cm->src_port_path() << " -> " << cm->dst_port_path() << endl;

	CountedPtr<ConnectionModel> existing = get_connection(cm->src_port_path(), cm->dst_port_path());

	if (existing) {
		return;
	}

	NodeModel* src_node = (cm->src_port_path().parent() == path())
		? this : get_node(cm->src_port_path().parent().name()).get();
	PortModel* src_port = (src_node == NULL) ? NULL : src_node->get_port(cm->src_port_path().name()).get();
	NodeModel* dst_node = (cm->dst_port_path().parent() == path())
		? this : get_node(cm->dst_port_path().parent().name()).get();
	PortModel* dst_port = (dst_node == NULL) ? NULL : dst_node->get_port(cm->dst_port_path().name()).get();
	
	assert(src_port != NULL);
	assert(dst_port != NULL);
	
	// Find source port pointer to 'resolve' connection if necessary
	if (cm->src_port() != NULL)
		assert(cm->src_port() == src_port);
	else
		cm->set_src_port(src_port);
	
	// Find dest port pointer to 'resolve' connection if necessary
	if (cm->dst_port() != NULL)
		assert(cm->dst_port() == dst_port);
	else
		cm->set_dst_port(dst_port);
	
	assert(cm->src_port() != NULL);
	assert(cm->dst_port() != NULL);

	m_connections.push_back(cm);

	new_connection_sig.emit(cm);
}


void
PatchModel::remove_connection(const string& src_port_path, const string& dst_port_path)
{
	for (list<CountedPtr<ConnectionModel> >::iterator i = m_connections.begin(); i != m_connections.end(); ++i) {
		CountedPtr<ConnectionModel> cm = (*i);
		if (cm->src_port_path() == src_port_path && cm->dst_port_path() == dst_port_path) {
			m_connections.erase(i); // cuts our reference
			assert(!get_connection(src_port_path, dst_port_path)); // no duplicates
			removed_connection_sig.emit(src_port_path, dst_port_path);
			return;
		}
	}
	cerr << "[PatchModel::remove_connection] WARNING: Failed to find connection " <<
		src_port_path << " -> " << dst_port_path << endl;
}


void
PatchModel::enable()
{
	if (!m_enabled) {
		m_enabled = true;
		enabled_sig.emit();
	}
}


void
PatchModel::disable()
{
	if (m_enabled) {
		m_enabled = false;
		disabled_sig.emit();
	}
}


bool
PatchModel::polyphonic() const
{
	return (!m_parent)
		? (m_poly > 1)
		: (m_poly > 1) && m_poly == ((PatchModel*)m_parent.get())->poly() && m_poly > 1;
}


} // namespace LibOmClient
