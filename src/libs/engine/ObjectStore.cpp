/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
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

#include "ObjectStore.h"
#include "Om.h"
#include "OmApp.h"
#include "Patch.h"
#include "Node.h"
#include "Port.h"
#include "List.h"
#include "util/Path.h"
#include "Tree.h"

namespace Om {


/** Find the Patch at the given path.
 */
Patch*
ObjectStore::find_patch(const Path& path) 
{
	OmObject* const object = find(path);
	return (object == NULL) ? NULL : object->as_patch();
}


/** Find the Node at the given path.
 */
Node*
ObjectStore::find_node(const Path& path) 
{
	OmObject* const object = find(path);
	return (object == NULL) ? NULL : object->as_node();
}


/** Find the Port at the given path.
 */
Port*
ObjectStore::find_port(const Path& path) 
{
	OmObject* const object = find(path);
	return (object == NULL) ? NULL : object->as_port();
}


/** Find the Object at the given path.
 */
OmObject*
ObjectStore::find(const Path& path)
{
	return m_objects.find(path);
}


/** Add an object to the store. Not realtime safe.
 */
void
ObjectStore::add(OmObject* o)
{
	//cerr << "[ObjectStore] Adding " << o->path() << endl;
	m_objects.insert(new TreeNode<OmObject*>(o->path(), o));
}


/** Add an object to the store. Not realtime safe.
 */
void
ObjectStore::add(TreeNode<OmObject*>* tn)
{
	//cerr << "[ObjectStore] Adding " << tn->key() << endl;
	m_objects.insert(tn);
}


/** Remove a patch from the store.
 *
 * It it the caller's responsibility to delete the returned ListNode.
 * 
 * @returns TreeNode containing object removed on success, NULL if not found.
 */
TreeNode<OmObject*>*
ObjectStore::remove(const string& path)
{
	TreeNode<OmObject*>* const removed = m_objects.remove(path);

	if (removed == NULL)
		cerr << "[ObjectStore] WARNING: Removing " << path << " failed." << endl;
	//else
	//	cerr << "[ObjectStore] Removed " << path << endl;
	
	return removed;
}


} // namespace Om
