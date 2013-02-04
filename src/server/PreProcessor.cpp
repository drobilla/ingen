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

#include <stdexcept>
#include <iostream>

#include "Event.hpp"
#include "PostProcessor.hpp"
#include "PreProcessor.hpp"
#include "ProcessContext.hpp"
#include "ThreadManager.hpp"

using namespace std;

namespace Ingen {
namespace Server {

PreProcessor::PreProcessor()
	: _sem(0)
	, _head(NULL)
	, _prepared_back(NULL)
	, _tail(NULL)
	, _exit_flag(false)
	, _thread(&PreProcessor::run, this)
{}

PreProcessor::~PreProcessor()
{
	_exit_flag = true;
	_sem.post();
	_thread.join();
}

void
PreProcessor::event(Event* const ev)
{
	// TODO: Probably possible to make this lock-free with CAS
	ThreadManager::assert_not_thread(THREAD_IS_REAL_TIME);
	std::lock_guard<std::mutex> lock(_mutex);

	assert(!ev->is_prepared());
	assert(!ev->next());

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
PreProcessor::process(ProcessContext& context, PostProcessor& dest, bool limit)
{
	Event* const head = _head.load();
	if (!head) {
		return 0;
	}

	/* Limit the maximum number of events to process each cycle to ensure the
	   process callback is real-time safe.  TODO: Parameterize this and/or
	   figure out a good default value. */
	const size_t MAX_EVENTS_PER_CYCLE = context.nframes() / 4;

	size_t n_processed = 0;
	Event* ev          = head;
	Event* last        = ev;
	while (ev && ev->is_prepared() && ev->time() < context.end()) {
		if (ev->time() < context.start()) {
			// Didn't get around to executing in time, oh well...
			ev->set_time(context.start());
		}
		ev->execute(context);
		last = ev;
		ev   = ev->next();
		++n_processed;
		if (limit && (n_processed > MAX_EVENTS_PER_CYCLE)) {
			break;
		}
	}

	if (n_processed > 0) {
		Event* next = (Event*)last->next();
		last->next(NULL);
		dest.append(context, head, last);

		// Since _head was not NULL, we know it hasn't been changed since
		_head = next;

		/* If next is NULL, then _tail may now be invalid.  However, in this
		   case _head is also NULL so event() will not read _tail.  Since it
		   could cause a race with event(), _tail is not set to NULL here. */
	}

	return n_processed;
}

void
PreProcessor::run()
{
	ThreadManager::set_flag(THREAD_PRE_PROCESS);
	while (_sem.wait() && !_exit_flag) {
		Event* const ev = _prepared_back.load();
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
