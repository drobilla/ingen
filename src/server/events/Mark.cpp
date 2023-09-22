/*
  This file is part of Ingen.
  Copyright 2016 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "events/Mark.hpp"

#include "ingen/Message.hpp"
#include "ingen/Status.hpp"

#include "CompiledGraph.hpp"
#include "Engine.hpp"
#include "GraphImpl.hpp"
#include "PreProcessContext.hpp"
#include "UndoStack.hpp"

#include <cassert>
#include <memory>
#include <unordered_set>
#include <utility>

namespace ingen::server::events {

Mark::Mark(Engine&                           engine,
           const std::shared_ptr<Interface>& client,
           SampleCount                       timestamp,
           const ingen::BundleBegin&         msg)
	: Event(engine, client, msg.seq, timestamp)
	, _type(Type::BUNDLE_BEGIN)
	, _depth(-1)
{}

Mark::Mark(Engine&                           engine,
           const std::shared_ptr<Interface>& client,
           SampleCount                       timestamp,
           const ingen::BundleEnd&           msg)
	: Event(engine, client, msg.seq, timestamp)
	, _type(Type::BUNDLE_END)
	, _depth(-1)
{}

Mark::~Mark() = default;

void
Mark::mark(PreProcessContext&)
{
	const std::unique_ptr<UndoStack>& stack = ((_mode == Mode::UNDO)
	                                           ? _engine.redo_stack()
	                                           : _engine.undo_stack());

	switch (_type) {
	case Type::BUNDLE_BEGIN:
		_depth = stack->start_entry();
		break;
	case Type::BUNDLE_END:
		_depth = stack->finish_entry();
		break;
	}
}

bool
Mark::pre_process(PreProcessContext& ctx)
{
	if (_depth < 0) {
		mark(ctx);
	}

	switch (_type) {
	case Type::BUNDLE_BEGIN:
		ctx.set_in_bundle(true);
		break;
	case Type::BUNDLE_END:
		ctx.set_in_bundle(false);
		if (!ctx.dirty_graphs().empty()) {
			for (GraphImpl* g : ctx.dirty_graphs()) {
				auto cg = compile(*g);
				if (cg) {
					_compiled_graphs.emplace(g, std::move(cg));
				}
			}
			ctx.dirty_graphs().clear();
		}
		break;
	}

	return Event::pre_process_done(Status::SUCCESS);
}

void
Mark::execute(RunContext&)
{
	for (auto& g : _compiled_graphs) {
		g.second = g.first->swap_compiled_graph(std::move(g.second));
	}
}

void
Mark::post_process()
{
	respond();
}

Event::Execution
Mark::get_execution() const
{
	assert(_depth >= 0);
	if (!_engine.atomic_bundles()) {
		return Execution::NORMAL;
	}

	switch (_type) {
	case Type::BUNDLE_BEGIN:
		if (_depth == 1) {
			return Execution::BLOCK;
		}
		break;
	case Type::BUNDLE_END:
		if (_depth == 0) {
			return Execution::UNBLOCK;
		}
		break;
	}
	return Execution::NORMAL;
}

} // namespace ingen::server::events
