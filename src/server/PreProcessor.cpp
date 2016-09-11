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

#include <stdexcept>

#include "ingen/AtomSink.hpp"
#include "ingen/AtomWriter.hpp"

#include "Engine.hpp"
#include "Event.hpp"
#include "PostProcessor.hpp"
#include "PreProcessor.hpp"
#include "RunContext.hpp"
#include "ThreadManager.hpp"
#include "UndoStack.hpp"

using namespace std;

namespace Ingen {
namespace Server {

PreProcessor::PreProcessor(Engine& engine)
	: _engine(engine)
	, _sem(0)
	, _head(NULL)
	, _prepared_back(NULL)
	, _tail(NULL)
	, _exit_flag(false)
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
	std::lock_guard<std::mutex> lock(_mutex);

	assert(!ev->is_prepared());
	assert(!ev->next());
	ev->set_mode(mode);

	/* Note that tail is only used here, not in process().  The head must be
	   checked first here, since if it is NULL the tail pointer is junk. */
	Event* const head = _head.load();
	if (!head) {
		_head = ev;
		_tail = ev;
	} else {
		_tail.load()->next(ev);
		_tail = ev;
	}

	if (!_prepared_back.load()) {
		_prepared_back = ev;
	}

	_sem.post();
}

unsigned
PreProcessor::process(RunContext& context, PostProcessor& dest, size_t limit)
{
	Event* const head        = _head.load();
	size_t       n_processed = 0;
	Event*       ev          = head;
	Event*       last        = ev;
	while (ev && ev->is_prepared() && ev->time() < context.end()) {
		if (ev->time() < context.start()) {
			// Didn't get around to executing in time, oh well...
			ev->set_time(context.start());
		}
		ev->execute(context);
		last = ev;
		ev   = ev->next();
		++n_processed;
		if (limit && n_processed >= limit) {
			break;
		}
	}

	if (n_processed > 0) {
		Event* next = (Event*)last->next();
		last->next(NULL);
		dest.append(context, head, last);

		// Since _head was not NULL, we know it hasn't been changed since
		_head = next;

		/* If next is NULL, then _tail may now be invalid.  However, it would cause
		   a race to reset _tail here.  Instead, append() checks only _head for
		   emptiness, and resets the tail appropriately. */
	}

	return n_processed;
}

void
PreProcessor::run()
{
	UndoStack& undo_stack = *_engine.undo_stack();
	UndoStack& redo_stack = *_engine.redo_stack();
	AtomWriter undo_writer(
		_engine.world()->uri_map(), _engine.world()->uris(), undo_stack);
	AtomWriter redo_writer(
		_engine.world()->uri_map(), _engine.world()->uris(), redo_stack);

	ThreadManager::set_flag(THREAD_PRE_PROCESS);
	while (!_exit_flag) {
		if (!_sem.timed_wait(1000)) {
			continue;
		}

		Event* const ev = _prepared_back.load();
		if (!ev) {
			return;
		}

		assert(!ev->is_prepared());
		if (ev->pre_process()) {
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

		_prepared_back = (Event*)ev->next();
	}
}

} // namespace Server
} // namespace Ingen
