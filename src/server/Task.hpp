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

#ifndef INGEN_ENGINE_TASK_HPP
#define INGEN_ENGINE_TASK_HPP

#include <atomic>
#include <cassert>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <utility>

namespace ingen {
namespace server {

class BlockImpl;
class RunContext;

class Task {
public:
	enum class Mode {
		SINGLE,      ///< Single block to run
		SEQUENTIAL,  ///< Elements must be run sequentially in order
		PARALLEL     ///< Elements may be run in any order in parallel
	};

	Task(Mode mode, BlockImpl* block = nullptr)
		: _block(block)
		, _mode(mode)
		, _done_end(0)
		, _next(0)
		, _done(false)
	{
		assert(!(mode == Mode::SINGLE && !block));
	}

	Task(Task&& task)
		: _children(std::move(task._children))
		, _block(task._block)
		, _mode(task._mode)
		, _done_end(task._done_end)
		, _next(task._next.load())
		, _done(task._done.load())
	{}

	Task& operator=(Task&& task)
	{
		_children = std::move(task._children);
		_block    = task._block;
		_mode     = task._mode;
		_done_end = task._done_end;
		_next     = task._next.load();
		_done     = task._done.load();
		return *this;
	}

	/** Run task in the given context. */
	void run(RunContext& context);

	/** Pretty print task to the given stream (recursively). */
	void dump(std::function<void (const std::string&)> sink, unsigned indent, bool first) const;

	/** Return true iff this is an empty task. */
	bool empty() const { return _mode != Mode::SINGLE && _children.empty(); }

	/** Simplify task expression. */
	static std::unique_ptr<Task> simplify(std::unique_ptr<Task>&& task);

	/** Steal a child task from this task (succeeds for PARALLEL only). */
	Task* steal(RunContext& context);

	/** Prepend a child to this task. */
	void push_front(Task&& task) {
		_children.emplace_front(std::unique_ptr<Task>(new Task(std::move(task))));
	}

	Mode       mode()  const { return _mode; }
	BlockImpl* block() const { return _block; }
	bool       done()  const { return _done; }

	void set_done(bool done) { _done = done; }

private:
	using Children = std::deque<std::unique_ptr<Task>>;

	Task(const Task&) = delete;
	Task& operator=(const Task&) = delete;

	Task* get_task(RunContext& context);

	void append(std::unique_ptr<Task>&& t) {
		_children.emplace_back(std::move(t));
	}

	Children              _children;  ///< Vector of child tasks
	BlockImpl*            _block;     ///< Used for SINGLE only
	Mode                  _mode;      ///< Execution mode
	unsigned              _done_end;  ///< Index of rightmost done sub-task
	std::atomic<unsigned> _next;      ///< Index of next sub-task
	std::atomic<bool>     _done;      ///< Completion phase
};

} // namespace server
} // namespace ingen

#endif // INGEN_ENGINE_TASK_HPP
