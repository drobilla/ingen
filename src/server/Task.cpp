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
		for (const auto& task : _children) {
			task->run(context);
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
		if (i < _children.size()) {
			return _children[i].get();
		}
	}

	return nullptr;
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
		while (_done_end < _children.size() && _children[_done_end]->done()) {
			++_done_end;
		}

		if (_done_end >= _children.size()) {
			return nullptr;  // All child tasks are finished
		}

		// All child tasks claimed, but some are unfinished, steal a task
		if ((t = context.engine().steal_task(context.id() + 1))) {
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

	for (auto t = task->_children.begin(); t != task->_children.end();) {
		*t = simplify(std::move(*t));
		if ((*t)->empty()) {
			// Empty task, erase
			t = task->_children.erase(t);
		} else if ((*t)->mode() == task->_mode) {
			// Subtask with the same type, fold child into parent
			std::unique_ptr<Task> child(std::move(*t));
			t = task->_children.erase(t);
			t = task->_children.insert(
				t,
				std::make_move_iterator(child->_children.begin()),
				std::make_move_iterator(child->_children.end()));
		} else {
			++t;
		}
	}

	if (task->_children.size() == 1) {
		return std::move(task->_children.front());
	}

	return std::move(task);
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
		for (size_t i = 0; i < _children.size(); ++i) {
			_children[i]->dump(sink, indent + 5, i == 0);
		}
		sink(")");
	}
}

} // namespace Server
} // namespace Ingen
