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

#ifndef OBJECTSTORE_H
#define OBJECTSTORE_H

#include <string>
#include <raul/PathTable.hpp>
#include <raul/SharedPtr.hpp>

using std::string;
using namespace Raul;

namespace Ingen {

namespace Shared { class GraphObject; }

class PatchImpl;
class NodeImpl;
class PortImpl;
class GraphObjectImpl;


/** Storage for all GraphObjects (tree of GraphObject's sorted by path).
 *
 * All looking up in pre_process() methods (and anything else that isn't in-band
 * with the audio thread) should use this (to read and modify the GraphObject
 * tree).
 *
 * Searching with find*() is fast (O(log(n)) binary search on contiguous
 * memory) and realtime safe, but modification (add or remove) are neither.
 */
class ObjectStore
{
public:
	typedef Raul::PathTable< SharedPtr<Shared::GraphObject> > Objects;

	PatchImpl*       find_patch(const Path& path);
	NodeImpl*        find_node(const Path& path);
	PortImpl*        find_port(const Path& path);
	GraphObjectImpl* find_object(const Path& path);
	
	Objects::iterator find(const Path& path) { return _objects.find(path); }
	
	void add(GraphObjectImpl* o);
	void add(const Table<Path, SharedPtr<Shared::GraphObject> >& family);
	//void add(TreeNode<GraphObjectImpl*>* o);

	Table<Path, SharedPtr<Shared::GraphObject> > remove(const Path& path);
	Table<Path, SharedPtr<Shared::GraphObject> > remove(Objects::iterator i);

	const Objects& objects() const { return _objects; }
	Objects&       objects()       { return _objects; }

private:
	Objects _objects;
};


} // namespace Ingen

#endif // OBJECTSTORE
