/*
  This file is part of Ingen.
  Copyright 2015-2016 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "BlockImpl.hpp"
#include "Engine.hpp"
#include "Task.hpp"

namespace Ingen {
namespace Server {

void
Task::run(RunContext& context)
{
	switch (_mode) {
	case Mode::SINGLE:
		// fprintf(stderr, "%u run %s\n", context.id(), _block->path().c_str());
		_block->process(context);
		break;
	case Mode::SEQUENTIAL:
		for (Task& task : *this) {
			task.run(context);
		}
		break;
	case Mode::PARALLEL:
		// Initialize (not) done state of sub-tasks
		for (Task& task : *this) {
			task.set_done(false);
		}

		// Grab the first sub-task
		_next     = 0;
		_done_end = 0;
		Task* t = steal(context);

		// Allow other threads to steal sub-tasks
		context.set_task(this);
		context.engine().signal_tasks();

		// Run available tasks until this task is finished
		for (; t; t = get_task(context)) {
			t->run(context);
		}
		context.set_task(nullptr);
		break;
	}

	set_done(true);
}

Task*
Task::steal(RunContext& context)
{
	if (_mode == Mode::PARALLEL) {
		const unsigned i = _next++;
		if (i < size()) {
			return &(*this)[i];
		}
	}

	return NULL;
}

Task*
Task::get_task(RunContext& context)
{
	// Attempt to "steal" a task from ourselves
	Task* t = steal(context);
	if (t) {
		return t;
	}

	while (true) {
		// Push done end index as forward as possible
		for (; (*this)[_done_end].done(); ++_done_end) {}

		if (_done_end >= size()) {
			return NULL;  // All child tasks are finished
		}

		// All child tasks claimed, but some are unfinished, steal a task
		t = context.engine().steal_task(context.id() + 1);
		if (t) {
			return t;
		}

		/* All child tasks are claimed, and we failed to steal any tasks.  Spin
		   to prevent blocking, though it would probably be wiser to wait for a
		   signal in non-main threads, and maybe even in the main thread
		   depending on your real-time safe philosophy... more experimentation
		   here is needed. */
	}
}

void
Task::simplify()
{
	if (_mode != Mode::SINGLE) {
		for (std::vector<Task>::iterator t = begin(); t != end();) {
			t->simplify();
			if (t->mode() != Mode::SINGLE && t->empty()) {
				// Empty task, erase
				t = erase(t);
			} else if (t->mode() == _mode) {
				// Subtask with the same type, fold child into parent
				const Task child(*t);
				t = erase(t);
				t = insert(t, child.begin(), child.end());
			} else {
				++t;
			}
		}

		if (size() == 1) {
			const Task t(front());
			*this = t;
		}
	}
}

void
Task::dump(std::function<void (const std::string&)> sink, unsigned indent, bool first) const
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
		for (size_t i = 0; i < size(); ++i) {
			(*this)[i].dump(sink, indent + 5, i == 0);
		}
		sink(")");
	}
}

} // namespace Server
} // namespace Ingen
