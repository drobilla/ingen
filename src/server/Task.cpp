/*
  This file is part of Ingen.
  Copyright 2015-2017 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Task.hpp"

#include "BlockImpl.hpp"
#include "RunContext.hpp"

#include "raul/Path.hpp"

#include <cstddef>

namespace ingen {
namespace server {

void
Task::run(RunContext& ctx)
{
	switch (_mode) {
	case Mode::SINGLE:
		// fprintf(stderr, "%u run %s\n", context.id(), _block->path().c_str());
		_block->process(ctx);
		break;
	case Mode::SEQUENTIAL:
		for (const auto& task : _children) {
			task->run(ctx);
		}
		break;
	case Mode::PARALLEL:
		// Initialize (not) done state of sub-tasks
		for (const auto& task : _children) {
			task->set_done(false);
		}

		// Grab the first sub-task
		_next     = 0;
		_done_end = 0;
		Task* t = steal(ctx);

		// Allow other threads to steal sub-tasks
		ctx.claim_task(this);

		// Run available tasks until this task is finished
		for (; t; t = get_task(ctx)) {
			t->run(ctx);
		}
		ctx.claim_task(nullptr);
		break;
	}

	set_done(true);
}

Task*
Task::steal(RunContext&)
{
	if (_mode == Mode::PARALLEL) {
		const unsigned i = _next++;
		if (i < _children.size()) {
			return _children[i].get();
		}
	}

	return nullptr;
}

Task*
Task::get_task(RunContext& ctx)
{
	// Attempt to "steal" a task from ourselves
	Task* t = steal(ctx);
	if (t) {
		return t;
	}

	while (true) {
		// Push done end index as forward as possible
		while (_done_end < _children.size() && _children[_done_end]->done()) {
			++_done_end;
		}

		if (_done_end >= _children.size()) {
			return nullptr;  // All child tasks are finished
		}

		// All child tasks claimed, but some are unfinished, steal a task
		if ((t = ctx.steal_task())) {
			return t;
		}

		/* All child tasks are claimed, and we failed to steal any tasks.  Spin
		   to prevent blocking, though it would probably be wiser to wait for a
		   signal in non-main threads, and maybe even in the main thread
		   depending on your real-time safe philosophy... more experimentation
		   here is needed. */
	}
}

std::unique_ptr<Task>
Task::simplify(std::unique_ptr<Task>&& task)
{
	if (task->mode() == Mode::SINGLE) {
		return std::move(task);
	}

	std::unique_ptr<Task> ret = std::unique_ptr<Task>(new Task(task->mode()));
	for (auto&& c : task->_children) {
		auto child = simplify(std::move(c));
		if (!child->empty()) {
			if (child->mode() == task->mode()) {
				// Merge child into parent
				for (auto&& grandchild : child->_children) {
					ret->append(std::move(grandchild));
				}
			} else {
				// Add child task
				ret->append(std::move(child));
			}
		}
	}

	if (ret->_children.size() == 1) {
		return std::move(ret->_children.front());
	}

	return ret;
}

void
Task::dump(const std::function<void(const std::string&)>& sink,
           unsigned                                       indent,
           bool                                           first) const
{
	if (!first) {
		sink("\n");
		for (unsigned i = 0; i < indent; ++i) {
			sink(" ");
		}
	}

	if (_mode == Mode::SINGLE) {
		sink(_block->path());
	} else {
		sink(((_mode == Mode::SEQUENTIAL) ? "(seq " : "(par "));
		for (size_t i = 0; i < _children.size(); ++i) {
			_children[i]->dump(sink, indent + 5, i == 0);
		}
		sink(")");
	}
}

} // namespace server
} // namespace ingen
