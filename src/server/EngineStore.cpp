/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <utility>
#include <vector>
#include "raul/log.hpp"
#include "raul/List.hpp"
#include "raul/PathTable.hpp"
#include "raul/TableImpl.hpp"
#include "EngineStore.hpp"
#include "PatchImpl.hpp"
#include "NodeImpl.hpp"
#include "PortImpl.hpp"
#include "ThreadManager.hpp"

#define LOG(s) s << "[EngineStore] "

using namespace std;

namespace Ingen {
namespace Server {

EngineStore::~EngineStore()
{
	clear();
}

/** Find the Patch at the given path.
 */
PatchImpl*
EngineStore::find_patch(const Raul::Path& path)
{
	GraphObjectImpl* const object = find_object(path);
	return dynamic_cast<PatchImpl*>(object);
}

/** Find the Node at the given path.
 */
NodeImpl*
EngineStore::find_node(const Raul::Path& path)
{
	GraphObjectImpl* const object = find_object(path);
	return dynamic_cast<NodeImpl*>(object);
}

/** Find the Port at the given path.
 */
PortImpl*
EngineStore::find_port(const Raul::Path& path)
{
	GraphObjectImpl* const object = find_object(path);
	return dynamic_cast<PortImpl*>(object);
}

/** Find the Object at the given path.
 */
GraphObjectImpl*
EngineStore::find_object(const Raul::Path& path)
{
	iterator i = find(path);
	return ((i == end()) ? NULL : dynamic_cast<GraphObjectImpl*>(i->second.get()));
}

/** Add an object to the store. Not realtime safe.
 */
void
EngineStore::add(GraphObject* obj)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);
	Store::add(obj);
}

/** Add a family of objects to the store. Not realtime safe.
 */
void
EngineStore::add(const Objects& table)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);
	cram(table);
}

/** Remove an object from the store.
 *
 * Returned is a vector containing all descendants of the object removed
 * including the object itself, in lexicographically sorted order by Path.
 */
SharedPtr<EngineStore::Objects>
EngineStore::remove(const Raul::Path& path)
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
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);

	if (object != end()) {
		iterator descendants_end = find_descendants_end(object);
		SharedPtr<Objects> removed = yank(object, descendants_end);

		return removed;

	} else {
		LOG(Raul::warn) << "Removing " << object->first << " failed." << endl;
		return SharedPtr<EngineStore>();
	}
}

/** Remove all children of an object from the store.
 *
 * Returned is a vector containing all descendants of the object removed
 * in lexicographically sorted order by Path.
 */
SharedPtr<EngineStore::Objects>
EngineStore::remove_children(const Raul::Path& path)
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
		LOG(Raul::warn) << "Removing children of " << object->first << " failed." << endl;
		return SharedPtr<EngineStore::Objects>();
	}

	return SharedPtr<EngineStore::Objects>();
}

} // namespace Server
} // namespace Ingen
