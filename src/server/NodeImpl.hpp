/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_ENGINE_NODEIMPL_HPP
#define INGEN_ENGINE_NODEIMPL_HPP

#include <cassert>
#include <cstddef>
#include <map>

#include "ingen/Node.hpp"
#include "ingen/Resource.hpp"
#include "raul/Deletable.hpp"
#include "raul/Path.hpp"

namespace Raul { class Maid; }

namespace Ingen {

namespace Shared { class URIs; }

namespace Server {

class BufferFactory;
class GraphImpl;
class RunContext;

/** An object on the audio graph (a Graph, Block, or Port).
 *
 * Each of these is a Raul::Deletable and so can be deleted in a realtime safe
 * way from anywhere, and they all have a map of variable for clients to store
 * arbitrary values in (which the engine puts no significance to whatsoever).
 *
 * \ingroup engine
 */
class NodeImpl : public Node
{
public:
	const Raul::Symbol& symbol() const { return _symbol; }

	Node*     graph_parent() const { return _parent; }
	NodeImpl* parent()       const { return _parent; }

	/** Rename */
	virtual void set_path(const Raul::Path& new_path) {
		_path = new_path;
		const char* const new_sym = new_path.symbol();
		if (new_sym[0] != '\0') {
			_symbol = Raul::Symbol(new_sym);
		}
		set_uri(path_to_uri(new_path));
	}

	const Atom& get_property(const URI& key) const;

	/** The Graph this object is a child of. */
	virtual GraphImpl* parent_graph() const;

	const Raul::Path& path() const { return _path; }

	/** Prepare for a new (external) polyphony value.
	 *
	 * Preprocessor thread, poly is actually applied by apply_poly.
	 * \return true on success.
	 */
	virtual bool prepare_poly(BufferFactory& bufs, uint32_t poly) = 0;

	/** Apply a new (external) polyphony value.
	 *
	 * \param context Process context (process thread only).
	 * \param poly Must be <= the most recent value passed to prepare_poly.
	 */
	virtual bool apply_poly(RunContext& context, uint32_t poly) = 0;

	/** Return true iff this is main (the top level Node).
	 *
	 * This is sometimes called "the root graph", but the term "main" is used
	 * to avoid ambiguity with the root path, since main does not have the path
	 * "/", but usually "/main" to leave namespace for non-node things.
	 */
	bool is_main() const { return !_parent; }

protected:
	NodeImpl(const Ingen::URIs&  uris,
	         NodeImpl*           parent,
	         const Raul::Symbol& symbol);

	NodeImpl*    _parent;
	Raul::Path   _path;
	Raul::Symbol _symbol;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_NODEIMPL_HPP
