/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#include "PatchModel.hpp"
#include "NodeModel.hpp"
#include "ConnectionModel.hpp"
#include <cassert>
#include <iostream>

using std::cerr; using std::cout; using std::endl;

namespace Ingen {
namespace Client {


void
PatchModel::set_path(const Path& new_path)
{
	throw;
#if 0
	// FIXME: haack
	if (new_path == "") {
		_path = "";
		return;
	}

	NodeModel::set_path(new_path);
	for (Children::iterator i = _children.begin(); i != _children.end(); ++i)
		(*i).second->set_path(_path +"/"+ (*i).second->path().name());
	
#ifdef DEBUG
	// Be sure connection paths are updated and sane
	for (list<SharedPtr<ConnectionModel> >::iterator j = _connections.begin();
			j != _connections.end(); ++j) {
		assert((*j)->src_port_path().parent().parent() == new_path);
		assert((*j)->src_port_path().parent().parent() == new_path);
	}
#endif
#endif
}


void
PatchModel::add_child(SharedPtr<ObjectModel> c)
{
	assert(c->parent().get() == this);
	
	ObjectModel::add_child(c);

	SharedPtr<PortModel> pm = PtrCast<PortModel>(c);
	if (pm) {
		add_port(pm);
		return;
	}
	
	SharedPtr<NodeModel> nm = PtrCast<NodeModel>(c);
	if (nm)
		new_node_sig.emit(nm);
}

SharedPtr<NodeModel>
PatchModel::get_node(const string& name) const
{
	return PtrCast<NodeModel>(get_child(name));
}


bool
PatchModel::remove_child(SharedPtr<ObjectModel> o)
{
	assert(o->path().is_child_of(_path));
	assert(o->parent().get() == this);

	SharedPtr<PortModel> pm = PtrCast<PortModel>(o);
	if (pm)
		remove_port(pm);
	
	// Remove any connections which referred to this object,
	// since they can't possibly exist anymore
	for (ConnectionList::iterator j = _connections.begin(); j != _connections.end() ; ) {

		list<SharedPtr<ConnectionModel> >::iterator next = j;
		++next;

		SharedPtr<ConnectionModel> cm = (*j);

		if (cm->src_port_path().parent() == o->path()
				|| cm->src_port_path() == o->path()
				|| cm->dst_port_path().parent() == o->path()
				|| cm->dst_port_path() == o->path()) {
			removed_connection_sig.emit(cm);
			_connections.erase(j); // cuts our reference
			assert(!get_connection(cm->src_port_path(), cm->dst_port_path())); // no duplicates
		}
		j = next;
	}

	if (ObjectModel::remove_child(o)) {
		SharedPtr<NodeModel> nm = PtrCast<NodeModel>(o);
		if (nm) {
			removed_node_sig.emit(nm);
		}
		return true;
	} else {
		return false;
	}
}

#if 0
void
PatchModel::remove_node(const string& name)
{
	assert(name.find("/") == string::npos);
	NodeModelMap::iterator i = _children.find(name);
	if (i != _children.end()) {
		//delete i->second;
		_children.erase(i);
		removed_node_sig.emit(name);
		i->second->parent().reset();
		return;
	}
	
	cerr << "[PatchModel::remove_node] " << _path << ": failed to find node " << name << endl;
}
#endif

void
PatchModel::clear()
{
	//for (list<SharedPtr<ConnectionModel> >::iterator j = _connections.begin(); j != _connections.end(); ++j)
	//	delete (*j);
	
	/*for (Children::iterator i = _children.begin(); i != _children.end(); ++i) {
		(*i).second->clear();
		//delete (*i).second;
	}*/
	
	_children.clear();
	_connections.clear();

	NodeModel::clear();

	assert(_children.empty());
	assert(_connections.empty());
	assert(_ports.empty());
}


/** Updated the map key of the given node.
 *
 * The NodeModel must already have it's new path set to @a new_path, or this will
 * abort with a fatal error.
 */
void
PatchModel::rename_node(const Path& old_path, const Path& new_path)
{
	cerr << "FIXME: node rename" << endl;
#if 0
	assert(old_path.parent() == path());
	assert(new_path.parent() == path());
	
	NodeModelMap::iterator i = _children.find(old_path.name());
	
	if (i != _children.end()) {
		SharedPtr<NodeModel> nm = (*i).second;
		for (list<SharedPtr<ConnectionModel> >::iterator j = _connections.begin(); j != _connections.end(); ++j) {
			if ((*j)->src_port_path().parent() == old_path)
				(*j)->src_port_path(new_path.base() + (*j)->src_port_path().name());
			if ((*j)->dst_port_path().parent() == old_path)
				(*j)->dst_port_path(new_path.base() + (*j)->dst_port_path().name());
		}
		_children.erase(i);
		assert(nm->path() == new_path);
		_children[new_path.name()] = nm;
		return;
	}
	
	cerr << "[PatchModel::rename_node] " << _path << ": failed to find node " << old_path << endl;
#endif
}


SharedPtr<ConnectionModel>
PatchModel::get_connection(const string& src_port_path, const string& dst_port_path) const
{
	for (list<SharedPtr<ConnectionModel> >::const_iterator i = _connections.begin(); i != _connections.end(); ++i)
		if ((*i)->src_port_path() == src_port_path && (*i)->dst_port_path() == dst_port_path)
			return (*i);
	return SharedPtr<ConnectionModel>();
}


/** Add a connection to this patch.
 *
 * A reference to  @a cm is taken, released on deletion or removal.
 * If @a cm only contains paths (not pointers to the actual ports), the ports
 * will be found and set.  The ports referred to not existing as children of
 * this patch is a fatal error.
 */
void
PatchModel::add_connection(SharedPtr<ConnectionModel> cm)
{
	// Store should have 'resolved' the connection already
	assert(cm);
	assert(cm->src_port());
	assert(cm->dst_port());
	assert(cm->src_port()->parent());
	assert(cm->dst_port()->parent());
	assert(cm->src_port()->parent().get() == this
	       || cm->src_port()->parent()->parent().get() == this);
	assert(cm->dst_port()->parent().get() == this
	       || cm->dst_port()->parent()->parent().get() == this);
	
	SharedPtr<ConnectionModel> existing = get_connection(cm->src_port_path(), cm->dst_port_path());
	
	if (existing) {
		assert(cm->src_port() == existing->src_port());
		assert(cm->dst_port() == existing->dst_port());
	} else {
		_connections.push_back(cm);
		new_connection_sig.emit(cm);
	}
}


void
PatchModel::remove_connection(const string& src_port_path, const string& dst_port_path)
{
	for (list<SharedPtr<ConnectionModel> >::iterator i = _connections.begin(); i != _connections.end(); ++i) {
		SharedPtr<ConnectionModel> cm = (*i);
		if (cm->src_port_path() == src_port_path && cm->dst_port_path() == dst_port_path) {
			removed_connection_sig.emit(cm);
			_connections.erase(i); // cuts our reference
			assert(!get_connection(src_port_path, dst_port_path)); // no duplicates
			return;
		}
	}

	cerr << "[PatchModel::remove_connection] WARNING: Failed to find connection " <<
		src_port_path << " -> " << dst_port_path << endl;
}


void
PatchModel::enable()
{
	if (!_enabled) {
		_enabled = true;
		enabled_sig.emit();
	}
}


void
PatchModel::disable()
{
	if (_enabled) {
		_enabled = false;
		disabled_sig.emit();
	}
}


bool
PatchModel::polyphonic() const
{
	return (_parent)
		? (_poly > 1) && _poly == PtrCast<PatchModel>(_parent)->poly() && _poly > 1
		: (_poly > 1);
}


} // namespace Client
} // namespace Ingen
