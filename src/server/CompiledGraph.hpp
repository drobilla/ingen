/*
  This file is part of Ingen.
  Copyright 2007-2017 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_ENGINE_COMPILEDGRAPH_HPP
#define INGEN_ENGINE_COMPILEDGRAPH_HPP

#include "Task.hpp"

#include "ingen/memory.hpp"
#include "raul/Maid.hpp"
#include "raul/Noncopyable.hpp"

#include <cstddef>
#include <set>
#include <string>

namespace ingen {
namespace server {

class BlockImpl;
class GraphImpl;
class RunContext;

/** A graph ``compiled'' into a quickly executable form.
 *
 * This is a flat sequence of nodes ordered such that the process thread can
 * execute the nodes in order and have nodes always executed before any of
 * their dependencies.
 */
class CompiledGraph : public Raul::Maid::Disposable
                    , public Raul::Noncopyable
{
public:
	static MPtr<CompiledGraph> compile(Raul::Maid& maid, GraphImpl& graph);

	void run(RunContext& ctx);

private:
	friend class Raul::Maid;  ///< Allow make_managed to construct

	CompiledGraph(GraphImpl* graph);

	using BlockSet = std::set<BlockImpl*>;

	void dump(const std::string& name) const;

	void compile_graph(GraphImpl* graph);

	void compile_block(BlockImpl* n,
	                   Task&      task,
	                   size_t     max_depth,
	                   BlockSet&  k);

	void compile_provider(const BlockImpl* root,
	                      BlockImpl*       block,
	                      Task&            task,
	                      size_t           max_depth,
	                      BlockSet&        k);

	std::unique_ptr<Task> _master;
};

inline MPtr<CompiledGraph> compile(Raul::Maid& maid, GraphImpl& graph)
{
	return CompiledGraph::compile(maid, graph);
}

} // namespace server
} // namespace ingen

#endif // INGEN_ENGINE_COMPILEDGRAPH_HPP
