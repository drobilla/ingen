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

#include <utility>
#include <vector>
#include <raul/List.hpp>
#include <raul/PathTable.hpp>
#include <raul/TableImpl.hpp>
#include "ObjectStore.hpp"
#include "PatchImpl.hpp"
#include "NodeImpl.hpp"
#include "PortImpl.hpp"
#include "ThreadManager.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {


/** Find the Patch at the given path.
 */
PatchImpl*
ObjectStore::find_patch(const Path& path) 
{
	GraphObjectImpl* const object = find_object(path);
	return dynamic_cast<PatchImpl*>(object);
}


/** Find the Node at the given path.
 */
NodeImpl*
ObjectStore::find_node(const Path& path) 
{
	GraphObjectImpl* const object = find_object(path);
	return dynamic_cast<NodeImpl*>(object);
}


/** Find the Port at the given path.
 */
PortImpl*
ObjectStore::find_port(const Path& path) 
{
	GraphObjectImpl* const object = find_object(path);
	return dynamic_cast<PortImpl*>(object);
}


/** Find the Object at the given path.
 */
GraphObjectImpl*
ObjectStore::find_object(const Path& path)
{
	Objects::iterator i = _objects.find(path);
	return ((i == _objects.end()) ? NULL : dynamic_cast<GraphObjectImpl*>(i->second.get()));
}


/** Add an object to the store. Not realtime safe.
 */
void
ObjectStore::add(GraphObjectImpl* o)
{
	assert(ThreadManager::current_thread_id() == THREAD_PRE_PROCESS);

	if (_objects.find(o->path()) != _objects.end()) {
		cerr << "[ObjectStore] ERROR: Attempt to add duplicate object " << o->path() << endl;
		return;
	}

	_objects.insert(make_pair(o->path(), o));

	NodeImpl* node = dynamic_cast<NodeImpl*>(o);
	if (node) {
		for (uint32_t i=0; i < node->num_ports(); ++i) {
			add(node->port_impl(i));
		}
	}
}


/** Add a family of objects to the store. Not realtime safe.
 */
void
ObjectStore::add(const Table<Path, SharedPtr<Shared::GraphObject> >& table)
{
	assert(ThreadManager::current_thread_id() == THREAD_PRE_PROCESS);

	//cerr << "[ObjectStore] Adding " << o[0].second->path() << endl;
	_objects.cram(table);
	
	/*cerr << "[ObjectStore] Adding Table:" << endl;
	for (Objects::const_iterator i = table.begin(); i != table.end(); ++i) {
		cerr << i->first << " = " << i->second->path() << endl;
	}*/
}


/** Remove an object from the store.
 *
 * Returned is a vector containing all descendants of the object removed
 * including the object itself, in lexicographically sorted order by Path.
 */
SharedPtr< Table<Path, SharedPtr<Shared::GraphObject> > >
ObjectStore::remove(const Path& path)
{
	return remove(_objects.find(path));
}


/** Remove an object from the store.
 *
 * Returned is a vector containing all descendants of the object removed
 * including the object itself, in lexicographically sorted order by Path.
 */
SharedPtr< Table<Path, SharedPtr<Shared::GraphObject> > >
ObjectStore::remove(Objects::iterator object)
{
	assert(ThreadManager::current_thread_id() == THREAD_PRE_PROCESS);
	
	if (object != _objects.end()) {
		Objects::iterator descendants_end = _objects.find_descendants_end(object);
		//cout << "[ObjectStore] Removing " << object->first << " {" << endl;
		SharedPtr< Table<Path, SharedPtr<Shared::GraphObject> > > removed
				= _objects.yank(object, descendants_end);
		/*for (Objects::iterator i = removed->begin(); i != removed->end(); ++i) {
			cout << "\t" << i->first << endl;
		}
		cout << "}" << endl;*/
	
		return removed;

	} else {
		cerr << "[ObjectStore] WARNING: Removing " << object->first << " failed." << endl;
		return SharedPtr<Objects>();
	}
}

	
/** Remove all children of an object from the store.
 *
 * Returned is a vector containing all descendants of the object removed
 * in lexicographically sorted order by Path.
 */
SharedPtr< Table<Path, SharedPtr<Shared::GraphObject> > >
ObjectStore::remove_children(const Path& path)
{
	return remove_children(_objects.find(path));
}


/** Remove all children of an object from the store.
 *
 * Returned is a vector containing all descendants of the object removed
 * in lexicographically sorted order by Path.
 */
SharedPtr< Table<Path, SharedPtr<Shared::GraphObject> > >
ObjectStore::remove_children(Objects::iterator object)
{
	if (object != _objects.end()) {
		Objects::iterator descendants_end = _objects.find_descendants_end(object);

		if (descendants_end != object) {
			Objects::iterator first_child = object;
			++first_child;
			return _objects.yank(first_child, descendants_end);
		}

	} else {
		cerr << "[ObjectStore] WARNING: Removing children of " << object->first << " failed." << endl;
		return SharedPtr<Objects>();
	}

	return SharedPtr<Objects>();
}


} // namespace Ingen
