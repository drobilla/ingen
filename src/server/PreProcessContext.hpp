/*
  This file is part of Ingen.
  Copyright 2007-2016 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_ENGINE_PREPROCESSCONTEXT_HPP
#define INGEN_ENGINE_PREPROCESSCONTEXT_HPP

#include "CompiledGraph.hpp"
#include "GraphImpl.hpp"

#include <unordered_set>

namespace Raul { class Maid; }

namespace ingen {
namespace server {

/** Event pre-processing context.
 *
 * \ingroup engine
 */
class PreProcessContext
{
public:
	using DirtyGraphs = std::unordered_set<GraphImpl*>;

	/** Return true iff an atomic bundle is currently being pre-processed. */
	bool in_bundle() const { return _in_bundle; }

	/** Set/unset atomic bundle flag. */
	void set_in_bundle(bool b) { _in_bundle = b; }

	/** Return true iff graph should be compiled now (after a change).
	 *
	 * This may return false when an atomic bundle is deferring compilation, in
	 * which case the graph is flagged as dirty for later compilation.
	 */
	bool must_compile(GraphImpl& graph) {
		if (!graph.enabled()) {
			return false;
		} else if (_in_bundle) {
			_dirty_graphs.insert(&graph);
			return false;
		} else {
			return true;
		}
	}

	/** Compile graph and return the result if necessary.
	 *
	 * This may return null when an atomic bundle is deferring compilation, in
	 * which case the graph is flagged as dirty for later compilation.
	 */
	MPtr<CompiledGraph> maybe_compile(Raul::Maid& maid, GraphImpl& graph) {
		if (must_compile(graph)) {
			return compile(maid, graph);
		}
		return MPtr<CompiledGraph>();
	}

	/** Return all graphs that require compilation after an atomic bundle. */
	const DirtyGraphs& dirty_graphs() const { return _dirty_graphs; }
	DirtyGraphs&       dirty_graphs()       { return _dirty_graphs; }

private:
	DirtyGraphs _dirty_graphs;
	bool        _in_bundle = false;
};

} // namespace server
} // namespace ingen

#endif // INGEN_ENGINE_PREPROCESSCONTEXT_HPP
