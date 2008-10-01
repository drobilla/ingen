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
#include "EngineStore.hpp"
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
EngineStore::find_patch(const Path& path) 
{
	GraphObjectImpl* const object = find_object(path);
	return dynamic_cast<PatchImpl*>(object);
}


/** Find the Node at the given path.
 */
NodeImpl*
EngineStore::find_node(const Path& path) 
{
	GraphObjectImpl* const object = find_object(path);
	return dynamic_cast<NodeImpl*>(object);
}


/** Find the Port at the given path.
 */
PortImpl*
EngineStore::find_port(const Path& path) 
{
	GraphObjectImpl* const object = find_object(path);
	return dynamic_cast<PortImpl*>(object);
}


/** Find the Object at the given path.
 */
GraphObjectImpl*
EngineStore::find_object(const Path& path)
{
	iterator i = find(path);
	return ((i == end()) ? NULL : dynamic_cast<GraphObjectImpl*>(i->second.get()));
}


/** Add an object to the store. Not realtime safe.
 */
void
EngineStore::add(GraphObject* obj)
{
	assert(ThreadManager::current_thread_id() == THREAD_PRE_PROCESS);
	Store::add(obj);
}


/** Add a family of objects to the store. Not realtime safe.
 */
void
EngineStore::add(const Objects& table)
{
	assert(ThreadManager::current_thread_id() == THREAD_PRE_PROCESS);

	//cerr << "[EngineStore] Adding " << o[0].second->path() << endl;
	cram(table);
	
	/*cerr << "[EngineStore] Adding Table:" << endl;
	for (const_iterator i = table.begin(); i != table.end(); ++i) {
		cerr << i->first << " = " << i->second->path() << endl;
	}*/
}


/** Remove an object from the store.
 *
 * Returned is a vector containing all descendants of the object removed
 * including the object itself, in lexicographically sorted order by Path.
 */
SharedPtr<EngineStore::Objects>
EngineStore::remove(const Path& path)
{
	return remove(find(path));
}


/** Remove an object from the store.
 *
 * Returned is a vector containing all descendants of the object removed
 * including the object itself, in lexicographically sorted order by Path.
 */
SharedPtr<EngineStore::Objects>
EngineStore::remove(iterator object)
{
	assert(ThreadManager::current_thread_id() == THREAD_PRE_PROCESS);
	
	if (object != end()) {
		iterator descendants_end = find_descendants_end(object);
		//cout << "[EngineStore] Removing " << object->first << " {" << endl;
		SharedPtr<Objects> removed = yank(object, descendants_end);
		/*for (iterator i = removed->begin(); i != removed->end(); ++i) {
			cout << "\t" << i->first << endl;
		}
		cout << "}" << endl;*/
	
		return removed;

	} else {
		cerr << "[EngineStore] WARNING: Removing " << object->first << " failed." << endl;
		return SharedPtr<EngineStore>();
	}
}

	
/** Remove all children of an object from the store.
 *
 * Returned is a vector containing all descendants of the object removed
 * in lexicographically sorted order by Path.
 */
SharedPtr<EngineStore::Objects>
EngineStore::remove_children(const Path& path)
{
	return remove_children(find(path));
}


/** Remove all children of an object from the store.
 *
 * Returned is a vector containing all descendants of the object removed
 * in lexicographically sorted order by Path.
 */
SharedPtr<EngineStore::Objects>
EngineStore::remove_children(iterator object)
{
	if (object != end()) {
		iterator descendants_end = find_descendants_end(object);
		if (descendants_end != object) {
			iterator first_child = object;
			++first_child;
			return yank(first_child, descendants_end);
		}
	} else {
		cerr << "[EngineStore] WARNING: Removing children of " << object->first << " failed." << endl;
		return SharedPtr<EngineStore::Objects>();
	}

	return SharedPtr<EngineStore::Objects>();
}


} // namespace Ingen
