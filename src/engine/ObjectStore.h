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


#ifndef OBJECTSTORE_H
#define OBJECTSTORE_H

#include <string>
#include "util/Path.h"
#include "Tree.h"
using std::string;

namespace Om {

class Patch;
class Node;
class Port;
class OmObject;


/** Storage for all OmObjects (tree of OmObject's sorted by path).
 *
 * All looking up in pre_process() methods (and anything else that isn't in-band
 * with the audio thread) should use this (to read and modify the OmObject
 * tree).
 */
class ObjectStore
{
public:
	Patch*    find_patch(const Path& path);
	Node*     find_node(const Path& path);
	Port*     find_port(const Path& path);
	OmObject* find(const Path& path);
	
	void                 add(OmObject* o);
	void                 add(TreeNode<OmObject*>* o);
	TreeNode<OmObject*>* remove(const string& key);
	
	const Tree<OmObject*>& objects() { return m_objects; }

private:
	Tree<OmObject*> m_objects;
};


} // namespace Om

#endif // OBJECTSTORE
