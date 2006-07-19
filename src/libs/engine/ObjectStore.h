/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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


#ifndef OBJECTSTORE_H
#define OBJECTSTORE_H

#include <string>
#include "util/Path.h"
#include "Tree.h"
using std::string;

namespace Ingen {

class Patch;
class Node;
class Port;
class GraphObject;


/** Storage for all GraphObjects (tree of GraphObject's sorted by path).
 *
 * All looking up in pre_process() methods (and anything else that isn't in-band
 * with the audio thread) should use this (to read and modify the GraphObject
 * tree).
 */
class ObjectStore
{
public:
	Patch*    find_patch(const Path& path);
	Node*     find_node(const Path& path);
	Port*     find_port(const Path& path);
	GraphObject* find(const Path& path);
	
	void                 add(GraphObject* o);
	void                 add(TreeNode<GraphObject*>* o);
	TreeNode<GraphObject*>* remove(const string& key);
	
	const Tree<GraphObject*>& objects() { return m_objects; }

private:
	Tree<GraphObject*> m_objects;
};


} // namespace Ingen

#endif // OBJECTSTORE
