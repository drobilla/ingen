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

#ifndef INGEN_ENGINE_OBJECTSTORE_HPP
#define INGEN_ENGINE_OBJECTSTORE_HPP

#include "raul/SharedPtr.hpp"

#include "ingen/shared/Store.hpp"

namespace Ingen {

class GraphObject;

namespace Server {

class BufferFactory;
class GraphObjectImpl;
class NodeImpl;
class PatchImpl;
class PortImpl;

/** Storage for all GraphObjects (tree of GraphObject's sorted by path).
 *
 * All looking up in pre_process() methods (and anything else that isn't in-band
 * with the audio thread) should use this (to read and modify the GraphObject
 * tree).
 *
 * Searching with find*() is fast (O(log(n)) binary search on contiguous
 * memory) and realtime safe, but modification (add or remove) are neither.
 */
class EngineStore : public Ingen::Shared::Store
{
public:
	EngineStore(SharedPtr<BufferFactory> f) : _factory(f) {}
	~EngineStore();

	SharedPtr<BufferFactory> buffer_factory() const { return _factory; }

	PatchImpl*       find_patch(const Raul::Path& path);
	NodeImpl*        find_node(const Raul::Path& path);
	PortImpl*        find_port(const Raul::Path& path);
	GraphObjectImpl* find_object(const Raul::Path& path);

	void add(Ingen::GraphObject* o);
	void add(const Objects& family);

	SharedPtr<Objects> remove(const Raul::Path& path);
	SharedPtr<Objects> remove(Objects::iterator i);
	SharedPtr<Objects> remove_children(const Raul::Path& path);
	SharedPtr<Objects> remove_children(Objects::iterator i);

private:
	/* This holds a reference to the BufferFactory since the objects stored
	   here refer to it, so the BufferFactory may only be deleted after the
	   EngineStore is emptied and deleted.
	*/

	SharedPtr<BufferFactory> _factory;
};

} // namespace Server
} // namespace Ingen

#endif // OBJECTSTORE
