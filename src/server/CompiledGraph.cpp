/*
  This file is part of Ingen.
  Copyright 2015-2025 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "CompiledGraph.hpp"

#include "BlockImpl.hpp"
#include "Engine.hpp"
#include "GraphImpl.hpp"
#include "ThreadManager.hpp"

#include <ingen/Atom.hpp>
#include <ingen/ColorContext.hpp>
#include <ingen/Configuration.hpp>
#include <ingen/Log.hpp>
#include <ingen/World.hpp>
#include <raul/Path.hpp>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <functional>
#include <limits>
#include <memory>
#include <numeric>
#include <utility>

namespace ingen::server {

/** Graph contains ambiguous feedback with no delay nodes. */
class FeedbackException : public std::exception
{
public:
	explicit FeedbackException(const BlockImpl* n)
	    : node(n)
	{}

	FeedbackException(const BlockImpl* n, const BlockImpl* r)
	    : node(n), root(r)
	{}

	const BlockImpl* node = nullptr;
	const BlockImpl* root = nullptr;
};

namespace {

bool
has_provider_with_many_dependants(const BlockImpl* n)
{
	return std::any_of(n->providers().begin(),
	                   n->providers().end(),
	                   [](const auto* p) {
		                   return p->dependants().size() > 1;
	                   });
}

} // namespace

CompiledGraph::CompiledGraph(GraphImpl* graph)
	: _master{std::make_unique<Task>(Task::Mode::SEQUENTIAL)}
{
	compile_graph(graph);
}

std::unique_ptr<CompiledGraph>
CompiledGraph::compile(GraphImpl& graph)
{
	try {
		return std::unique_ptr<CompiledGraph>(new CompiledGraph(&graph));
	} catch (const FeedbackException& e) {
		Log& log = graph.engine().log();
		if (e.node && e.root) {
			log.error("Feedback compiling %1% from %2%\n",
			          e.node->path(), e.root->path());
		} else {
			log.error("Feedback compiling %1%\n", e.node->path());
		}
		return nullptr;
	}
}

namespace {

size_t
num_unvisited_dependants(const BlockImpl* block)
{
	return std::count_if(block->dependants().begin(),
	                     block->dependants().end(),
	                     [](const auto* b) {
		                     return b->get_mark() == BlockImpl::Mark::UNVISITED;
	                     });
}

size_t
parallel_depth(const BlockImpl* block)
{
	if (has_provider_with_many_dependants(block)) {
		return 2;
	}

	const auto min_provider_depth =
	  std::accumulate(block->providers().begin(),
	                  block->providers().end(),
	                  std::numeric_limits<size_t>::max(),
	                  [](const size_t d, const BlockImpl* const b) {
		                  return std::min(d, parallel_depth(b));
	                  });

	return 2 + min_provider_depth;
}

} // namespace

void
CompiledGraph::compile_graph(GraphImpl* graph)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);

	// Start with sink nodes (no outputs, or connected only to graph outputs)
	std::set<BlockImpl*> blocks;
	for (auto& b : graph->blocks()) {
		// Mark all blocks as unvisited initially
		b.set_mark(BlockImpl::Mark::UNVISITED);

		if (b.dependants().empty()) {
			// Block has no dependants, add to initial working set
			blocks.insert(&b);
		}
	}

	// Keep compiling working set until all nodes are visited
	while (!blocks.empty()) {
		std::set<BlockImpl*> predecessors;

		// Calculate maximum sequential depth to consume this phase
		const auto depth =
		  std::accumulate(blocks.begin(),
		                  blocks.end(),
		                  std::numeric_limits<size_t>::max(),
		                  [](const size_t d, const BlockImpl* const b) {
			                  return std::min(d, parallel_depth(b));
		                  });

		Task par(Task::Mode::PARALLEL);
		for (auto* b : blocks) {
			assert(num_unvisited_dependants(b) == 0);
			Task seq(Task::Mode::SEQUENTIAL);
			compile_block(b, seq, depth, predecessors);
			par.push_front(std::move(seq));
		}
		_master->push_front(std::move(par));
		blocks = predecessors;
	}

	_master = Task::simplify(std::move(_master));

	if (graph->engine().world().conf().option("trace").get<int32_t>()) {
		const ColorContext ctx{stderr, ColorContext::Color::YELLOW};
		dump(graph->path());
	}
}

namespace {

/** Throw a FeedbackException iff `dependant` has `root` as a dependency. */
void
check_feedback(const BlockImpl* root, BlockImpl* provider)
{
	if (provider == root) {
		throw FeedbackException(root);
	}

	for (auto* p : provider->providers()) {
		const BlockImpl::Mark mark = p->get_mark();
		switch (mark) {
		case BlockImpl::Mark::UNVISITED:
			p->set_mark(BlockImpl::Mark::VISITING);
			check_feedback(root, p);
			break;
		case BlockImpl::Mark::VISITING:
			throw FeedbackException(p, root);
		case BlockImpl::Mark::VISITED:
			break;
		}
		p->set_mark(mark);
	}
}

} // namespace

void
CompiledGraph::compile_provider(const BlockImpl*      root,
                                BlockImpl*            block,
                                Task&                 task,
                                size_t                max_depth,
                                std::set<BlockImpl*>& k)
{
	if (block->dependants().size() > 1) {
		/* Provider has other dependants, so this is the tail of a sequential task.
		   Add provider to future working set and stop traversal. */
		check_feedback(root, block);
		if (num_unvisited_dependants(block) == 0) {
			k.insert(block);
		}
	} else if (max_depth > 0) {
		// Calling dependant has only this provider, add here
		if (task.mode() == Task::Mode::PARALLEL) {
			// Inside a parallel task, compile into a new sequential child
			Task seq(Task::Mode::SEQUENTIAL);
			compile_block(block, seq, max_depth, k);
			task.push_front(std::move(seq));
		} else {
			// Prepend to given sequential task
			compile_block(block, task, max_depth, k);
		}
	} else {
		if (num_unvisited_dependants(block) == 0) {
			k.insert(block);
		}
	}
}

void
CompiledGraph::compile_block(BlockImpl*            n,
                             Task&                 task,
                             size_t                max_depth,
                             std::set<BlockImpl*>& k)
{
	switch (n->get_mark()) {
	case BlockImpl::Mark::UNVISITED:
		n->set_mark(BlockImpl::Mark::VISITING);

		// Execute this task after the providers to follow
		task.push_front(Task(Task::Mode::SINGLE, n));

		if (n->providers().size() < 2) {
			// Single provider, prepend it to this sequential task
			for (auto* p : n->providers()) {
				compile_provider(n, p, task, max_depth - 1, k);
			}
		} else if (has_provider_with_many_dependants(n)) {
			// Stop recursion and enqueue providers for the next round
			for (auto* p : n->providers()) {
				if (num_unvisited_dependants(p) == 0) {
					k.insert(p);
				}
			}
		} else {
			// Multiple providers with only this node as dependant,
			// make a new parallel task to execute them
			Task par(Task::Mode::PARALLEL);
			for (auto* p : n->providers()) {
				compile_provider(n, p, par, max_depth - 1, k);
			}
			task.push_front(std::move(par));
		}
		n->set_mark(BlockImpl::Mark::VISITED);
		break;

	case BlockImpl::Mark::VISITING:
		throw FeedbackException(n);

	case BlockImpl::Mark::VISITED:
		break;
	}
}

void
CompiledGraph::run(RunContext& ctx)
{
	_master->run(ctx);
}

void
CompiledGraph::dump(const std::string& name) const
{
	auto sink = [](const std::string& s) {
		fwrite(s.c_str(), 1, s.size(), stderr);
	};

	sink("(compiled-graph ");
	sink(name);
	_master->dump(sink, 2, false);
	sink(")\n");
}

} // namespace ingen::server
