/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Event.hpp"
#include "PostProcessor.hpp"
#include "PreProcessor.hpp"
#include "ProcessContext.hpp"
#include "ThreadManager.hpp"

using namespace std;

namespace Ingen {
namespace Server {

PreProcessor::PreProcessor()
	: Raul::Thread("PreProcessor")
	, _sem(0)
{
	start();
}

PreProcessor::~PreProcessor()
{
}

void
PreProcessor::event(Event* const ev)
{
	// TODO: Probably possible to make this lock-free with CAS
	ThreadManager::assert_not_thread(THREAD_IS_REAL_TIME);
	Glib::Mutex::Lock lock(_mutex);

	assert(!ev->is_prepared());
	assert(!ev->next());

	Event* const head = _head.get();
	Event* const tail = _tail.get();

	if (!head) {
		_head = ev;
		_tail = ev;
	} else {
		_tail = ev;
		tail->next(ev);
	}

	if (!_prepared_back.get()) {
		_prepared_back = ev;
	}

	_sem.post();
}

unsigned
PreProcessor::process(ProcessContext& context, PostProcessor& dest, bool limit)
{
	if (!_head.get())
		return 0;

	/* Limit the maximum number of queued events to process per cycle.  This
	   makes the process callback (more) realtime-safe by preventing being
	   choked by events coming in faster than they can be processed.
	   FIXME: test this and figure out a good value
	*/
	const size_t MAX_QUEUED_EVENTS = context.nframes() / 32;

	size_t num_events_processed = 0;

	Event* ev   = _head.get();
	Event* last = ev;

	while (ev && ev->is_prepared() && ev->time() < context.end()) {
		if (ev->time() < context.start()) {
			// Didn't get around to executing in time, oh well...
			ev->set_time(context.start());
		}
		ev->execute(context);
		last = ev;
		ev = (Event*)ev->next();
		++num_events_processed;
		if (limit && (num_events_processed > MAX_QUEUED_EVENTS))
			break;
	}

	if (num_events_processed > 0) {
		Event* next = (Event*)last->next();
		last->next(NULL);
		assert(!last->next());
		dest.append(context, _head.get(), last);
		_head = next;
		if (!next)
			_tail = NULL;
	}

	return num_events_processed;
}

/** Pre-process a single event */
void
PreProcessor::_run()
{
	ThreadManager::set_flag(THREAD_PRE_PROCESS);
	while (_sem.wait() && !_exit_flag) {
		Event* const ev = _prepared_back.get();
		if (!ev) {
			return;
		}

		assert(!ev->is_prepared());
		ev->pre_process();
		assert(ev->is_prepared());

		_prepared_back = (Event*)ev->next();
	}
}

} // namespace Server
} // namespace Ingen
