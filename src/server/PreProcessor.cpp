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

#include "PreProcessor.hpp"

#include "Engine.hpp"
#include "Event.hpp"
#include "PostProcessor.hpp"
#include "PreProcessContext.hpp"
#include "RunContext.hpp"
#include "ThreadManager.hpp"
#include "UndoStack.hpp"

#include "ingen/Atom.hpp"
#include "ingen/AtomWriter.hpp"
#include "ingen/Configuration.hpp"
#include "ingen/World.hpp"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <memory>

namespace ingen::server {

PreProcessor::PreProcessor(Engine& engine)
	: _engine(engine)
	, _thread(&PreProcessor::run, this)
{}

PreProcessor::~PreProcessor()
{
	if (_thread.joinable()) {
		_exit_flag = true;
		_sem.post();
		_thread.join();
	}
}

void
PreProcessor::event(Event* const ev, Event::Mode mode)
{
	// TODO: Probably possible to make this lock-free with CAS
	ThreadManager::assert_not_thread(THREAD_IS_REAL_TIME);
	const std::lock_guard<std::mutex> lock{_mutex};

	assert(!ev->is_prepared());
	assert(!ev->next());
	ev->set_mode(mode);

	/* Note that tail is only used here, not in process().  The head must be
	   checked first here, since if it is null the tail pointer is junk. */
	Event* const head = _head.load();
	if (!head) {
		_head = ev;
		_tail = ev;
	} else {
		_tail.load()->next(ev);
		_tail = ev;
	}

	_sem.post();
}

unsigned
PreProcessor::process(RunContext& ctx, PostProcessor& dest, size_t limit)
{
	Event* const head        = _head.load();
	size_t       n_processed = 0;
	Event*       ev          = head;
	Event*       last        = ev;
	while (ev && ev->is_prepared()) {
		switch (_block_state.load()) {
		case BlockState::UNBLOCKED:
			break;
		case BlockState::PRE_BLOCKED:
			if (ev->get_execution() == Event::Execution::BLOCK) {
				_block_state = BlockState::BLOCKED;
			} else if (ev->get_execution() == Event::Execution::ATOMIC) {
				_block_state = BlockState::PROCESSING;
			}
			break;
		case BlockState::BLOCKED:
			break;
		case BlockState::PRE_UNBLOCKED:
			assert(ev->get_execution() == Event::Execution::BLOCK);
			if (ev->get_execution() == Event::Execution::BLOCK) {
				_block_state = BlockState::PROCESSING;
			}
			break;
		case BlockState::PROCESSING:
			if (ev->get_execution() == Event::Execution::UNBLOCK) {
				_block_state = BlockState::UNBLOCKED;
			}
		}

		if (_block_state == BlockState::BLOCKED) {
			break; // Waiting for PRE_UNBLOCKED
		}

		if (ev->time() < ctx.start()) {
			ev->set_time(ctx.start()); // Too late, nudge to context start
		} else if (_block_state != BlockState::PROCESSING &&
		           ev->time() >= ctx.end()) {
			break; // Event is for a future cycle
		}

		// Execute event
		ev->execute(ctx);
		++n_processed;

		// Unblock pre-processing if this is a non-bundled atomic event
		if (ev->get_execution() == Event::Execution::ATOMIC) {
			assert(_block_state.load() == BlockState::PROCESSING);
			_block_state = BlockState::UNBLOCKED;
		}

		// Move to next event
		last = ev;
		ev   = ev->next();

		if (_block_state != BlockState::PROCESSING &&
		    limit && n_processed >= limit) {
			break;
		}
	}

	if (n_processed > 0) {
#ifndef NDEBUG
		const Engine& engine = ctx.engine();
		if (engine.world().conf().option("trace").get<int32_t>()) {
			const uint64_t start = engine.cycle_start_time(ctx);
			const uint64_t end   = engine.current_time();
			fprintf(stderr, "Processed %zu events in %u us\n",
			        n_processed, static_cast<unsigned>(end - start));
		}
#endif

		auto* next = static_cast<Event*>(last->next());
		last->next(nullptr);
		dest.append(ctx, head, last);

		// Since _head was not null, we know it hasn't been changed since
		_head = next;

		/* If next is null, then _tail may now be invalid.  However, it would cause
		   a race to reset _tail here.  Instead, append() checks only _head for
		   emptiness, and resets the tail appropriately. */
	}

	return n_processed;
}

void
PreProcessor::run()
{
	PreProcessContext ctx;

	UndoStack& undo_stack = *_engine.undo_stack();
	UndoStack& redo_stack = *_engine.redo_stack();
	AtomWriter undo_writer(
		_engine.world().uri_map(), _engine.world().uris(), undo_stack);
	AtomWriter redo_writer(
		_engine.world().uri_map(), _engine.world().uris(), redo_stack);

	ThreadManager::set_flag(THREAD_PRE_PROCESS);

	Event* back = nullptr;
	while (!_exit_flag) {
		if (!_sem.timed_wait(std::chrono::seconds(1))) {
			continue;
		}

		if (!back) {
			// Ran off end, find new unprepared back
			back = _head;
			while (back && back->is_prepared()) {
				back = back->next();
			}
		}

		Event* const ev = back;
		if (!ev) {
			continue;
		}

		// Set block state before enqueueing event
		ev->mark(ctx);
		switch (ev->get_execution()) {
		case Event::Execution::NORMAL:
			break;
		case Event::Execution::ATOMIC:
			assert(_block_state == BlockState::UNBLOCKED);
			_block_state = BlockState::PRE_BLOCKED;
			break;
		case Event::Execution::BLOCK:
			assert(_block_state == BlockState::UNBLOCKED);
			_block_state = BlockState::PRE_BLOCKED;
			break;
		case Event::Execution::UNBLOCK:
			wait_for_block_state(BlockState::BLOCKED);
			_block_state = BlockState::PRE_UNBLOCKED;
		}

		// Prepare event, allowing it to be processed
		assert(!ev->is_prepared());
		if (ev->pre_process(ctx)) {
			switch (ev->get_mode()) {
			case Event::Mode::NORMAL:
			case Event::Mode::REDO:
				undo_stack.start_entry();
				ev->undo(undo_writer);
				undo_stack.finish_entry();
				// undo_stack.save(stderr);
				break;
			case Event::Mode::UNDO:
				redo_stack.start_entry();
				ev->undo(redo_writer);
				redo_stack.finish_entry();
				// redo_stack.save(stderr, "redo");
				break;
			}
		}
		assert(ev->is_prepared());

		// Wait for process() if necessary
		if (ev->get_execution() == Event::Execution::ATOMIC) {
			wait_for_block_state(BlockState::UNBLOCKED);
		}

		back = static_cast<Event*>(ev->next());
	}
}

} // namespace ingen::server
