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

#include "Engine.hpp"
#include "PreProcessContext.hpp"
#include "UndoStack.hpp"
#include "events/Mark.hpp"

namespace Ingen {
namespace Server {
namespace Events {

Mark::Mark(Engine&                   engine,
           SPtr<Interface>           client,
           SampleCount               timestamp,
           const Ingen::BundleBegin& msg)
	: Event(engine, client, msg.seq, timestamp)
	, _type(Type::BUNDLE_BEGIN)
	, _depth(0)
{}

Mark::Mark(Engine&                 engine,
           SPtr<Interface>         client,
           SampleCount             timestamp,
           const Ingen::BundleEnd& msg)
	: Event(engine, client, msg.seq, timestamp)
	, _type(Type::BUNDLE_END)
	, _depth(0)
{}

bool
Mark::pre_process(PreProcessContext& ctx)
{
	const UPtr<UndoStack>& stack = ((_mode == Mode::UNDO)
	                                ? _engine.redo_stack()
	                                : _engine.undo_stack());

	switch (_type) {
	case Type::BUNDLE_BEGIN:
		ctx.set_in_bundle(true);
		_depth = stack->start_entry();
		break;
	case Type::BUNDLE_END:
		_depth = stack->finish_entry();
		ctx.set_in_bundle(false);
		if (!ctx.dirty_graphs().empty()) {
			for (GraphImpl* g : ctx.dirty_graphs()) {
				MPtr<CompiledGraph> cg = compile(*_engine.maid(), *g);
				if (cg) {
					_compiled_graphs.insert(std::make_pair(g, std::move(cg)));
				}
			}
			ctx.dirty_graphs().clear();
		}
		break;
	}

	return Event::pre_process_done(Status::SUCCESS);
}

void
Mark::execute(RunContext& context)
{
	for (auto& g : _compiled_graphs) {
		g.first->set_compiled_graph(std::move(g.second));
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

} // namespace Events
} // namespace Server
} // namespace Ingen
