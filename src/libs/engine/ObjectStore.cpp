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

#include "ObjectStore.h"
#include "Patch.h"
#include "Node.h"
#include "Port.h"
#include "List.h"
#include "raul/Path.h"
#include "Tree.h"

namespace Ingen {


/** Find the Patch at the given path.
 */
Patch*
ObjectStore::find_patch(const Path& path) 
{
	GraphObject* const object = find(path);
	return dynamic_cast<Patch*>(object);
}


/** Find the Node at the given path.
 */
Node*
ObjectStore::find_node(const Path& path) 
{
	GraphObject* const object = find(path);
	return dynamic_cast<Node*>(object);
}


/** Find the Port at the given path.
 */
Port*
ObjectStore::find_port(const Path& path) 
{
	GraphObject* const object = find(path);
	return dynamic_cast<Port*>(object);
}


/** Find the Object at the given path.
 */
GraphObject*
ObjectStore::find(const Path& path)
{
	return _objects.find(path);
}


/** Add an object to the store. Not realtime safe.
 */
void
ObjectStore::add(GraphObject* o)
{
	cerr << "[ObjectStore] Adding " << o->path() << endl;
	_objects.insert(new TreeNode<GraphObject*>(o->path(), o));
}


/** Add an object to the store. Not realtime safe.
 */
void
ObjectStore::add(TreeNode<GraphObject*>* tn)
{
	cerr << "[ObjectStore] Adding " << tn->key() << endl;
	_objects.insert(tn);
}


/** Remove a patch from the store.
 *
 * It it the caller's responsibility to delete the returned ListNode.
 * 
 * @returns TreeNode containing object removed on success, NULL if not found.
 */
TreeNode<GraphObject*>*
ObjectStore::remove(const string& path)
{
	TreeNode<GraphObject*>* const removed = _objects.remove(path);

	if (removed == NULL)
		cerr << "[ObjectStore] WARNING: Removing " << path << " failed." << endl;
	else
		cerr << "[ObjectStore] Removed " << path << endl;
	
	return removed;
}


} // namespace Ingen
