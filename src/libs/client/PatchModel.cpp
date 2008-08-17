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
#include "ClientStore.hpp"
#include <cassert>
#include <iostream>

using std::cerr; using std::cout; using std::endl;

namespace Ingen {
namespace Client {


void
PatchModel::add_child(SharedPtr<ObjectModel> c)
{
	assert(c->parent().get() == this);

	SharedPtr<PortModel> pm = PtrCast<PortModel>(c);
	if (pm) {
		add_port(pm);
		return;
	}
	
	SharedPtr<NodeModel> nm = PtrCast<NodeModel>(c);
	if (nm)
		signal_new_node.emit(nm);
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
	for (Connections::iterator j = _connections.begin(); j != _connections.end() ; ) {

		Connections::iterator next = j;
		++next;

		SharedPtr<ConnectionModel> cm = PtrCast<ConnectionModel>(*j);
		assert(cm);

		if (cm->src_port_path().parent() == o->path()
				|| cm->src_port_path() == o->path()
				|| cm->dst_port_path().parent() == o->path()
				|| cm->dst_port_path() == o->path()) {
			signal_removed_connection.emit(cm);
			_connections.erase(j); // cuts our reference
			assert(!get_connection(cm->src_port_path(), cm->dst_port_path())); // no duplicates
		}
		j = next;
	}

	SharedPtr<NodeModel> nm = PtrCast<NodeModel>(o);
	if (nm) 
		signal_removed_node.emit(nm);
		
	return true;
}


void
PatchModel::clear()
{
	_connections.clear();

	NodeModel::clear();

	assert(_connections.empty());
	assert(_ports.empty());
}


SharedPtr<ConnectionModel>
PatchModel::get_connection(const string& src_port_path, const string& dst_port_path) const
{
	for (Connections::const_iterator i = _connections.begin(); i != _connections.end(); ++i)
		if ((*i)->src_port_path() == src_port_path && (*i)->dst_port_path() == dst_port_path)
			return PtrCast<ConnectionModel>(*i);

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
	assert(cm->src_port_path() != cm->dst_port_path());
	assert(cm->src_port()->parent().get() == this
	       || cm->src_port()->parent()->parent().get() == this);
	assert(cm->dst_port()->parent().get() == this
	       || cm->dst_port()->parent()->parent().get() == this);
	
	SharedPtr<ConnectionModel> existing = get_connection(cm->src_port_path(), cm->dst_port_path());

	if (existing) {
		assert(cm->src_port() == existing->src_port());
		assert(cm->dst_port() == existing->dst_port());
	} else {
		_connections.push_back(new Connections::Node(cm));
		signal_new_connection.emit(cm);
	}
}


void
PatchModel::remove_connection(const string& src_port_path, const string& dst_port_path)
{
	for (Connections::iterator i = _connections.begin(); i != _connections.end(); ++i) {
		SharedPtr<ConnectionModel> cm = PtrCast<ConnectionModel>(*i);
		assert(cm);
		if (cm->src_port_path() == src_port_path && cm->dst_port_path() == dst_port_path) {
			signal_removed_connection.emit(cm);
			delete _connections.erase(i); // cuts our reference
			assert(!get_connection(src_port_path, dst_port_path)); // no duplicates
			return;
		}
	}

	cerr << "[PatchModel::remove_connection] WARNING: Failed to find connection " <<
		src_port_path << " -> " << dst_port_path << endl;
}


bool
PatchModel::enabled() const
{
	Variables::const_iterator i = _properties.find("ingen:enabled");
	return (i != _properties.end() && i->second.type() == Atom::BOOL && i->second.get_bool());
}
	

void
PatchModel::set_property(const string& key, const Atom& value)
{
	ObjectModel::set_property(key, value);
	if (key == "ingen:polyphony")
		_poly = value.get_int32();
}


bool
PatchModel::polyphonic() const
{
	return (_parent)
		? (_poly > 1) && _poly == PtrCast<PatchModel>(_parent)->poly() && _poly > 1
		: (_poly > 1);
}


unsigned
PatchModel::child_name_offset(ClientStore& store,
	                          SharedPtr<PatchModel> parent,
	                          const string& base_name)
{
	assert(Path::is_valid_name(base_name));
	unsigned offset = 0;

	while (true) {
		std::stringstream ss;
		ss << base_name;
		if (offset > 0)
			ss << "_" << offset;
		if (store.find(parent->path().base() + ss.str()) == store.end())
			break;
		else if (offset == 0)
			offset = 2;
		else
			++offset;
	}

	return offset;
}


} // namespace Client
} // namespace Ingen
