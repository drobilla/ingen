/* This file is part of Ingen.
 * Copyright (C) 2008-2009 David Robillard <http://drobilla.net>
 *
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <sys/mman.h>
#include "EventSource.hpp"
#include "QueuedEvent.hpp"
#include "PostProcessor.hpp"
#include "ThreadManager.hpp"
#include "ProcessContext.hpp"

using namespace std;

namespace Ingen {


EventSource::EventSource(size_t queue_size)
	: _blocking_semaphore(0)
{
	Thread::set_context(THREAD_PRE_PROCESS);
	set_name("EventSource");
}


EventSource::~EventSource()
{
	Thread::stop();
}


/** Push an unprepared event onto the queue.
 */
void
EventSource::push_queued(QueuedEvent* const ev)
{
	assert(!ev->is_prepared());
	Raul::List<Event*>::Node* node = new Raul::List<Event*>::Node(ev);
	_events.push_back(node);
	if (_prepared_back.get() == NULL)
		_prepared_back = node;

	whip();
}


/** Process all events for a cycle.
 *
 * Executed events will be pushed to @a dest.
 */
void
EventSource::process(PostProcessor& dest, ProcessContext& context, bool limit)
{
	ThreadManager::assert_thread(THREAD_PROCESS);

	if (_events.empty())
		return;

	/* Limit the maximum number of queued events to process per cycle.  This
	 * makes the process callback (more) realtime-safe by preventing being
	 * choked by events coming in faster than they can be processed.
	 * FIXME: test this and figure out a good value */
	const size_t MAX_QUEUED_EVENTS = context.nframes() / 32;

	size_t num_events_processed = 0;

	Raul::List<Event*>::Node* head = _events.head();
	Raul::List<Event*>::Node* tail = head;

	if (!head)
		return;
	
	QueuedEvent* ev = (QueuedEvent*)head->elem();

	while (ev && ev->is_prepared() && ev->time() < context.end()) {
		ev->execute(context);
		tail = head;
		head = head->next();
		++num_events_processed;
		if (limit && num_events_processed > MAX_QUEUED_EVENTS)
			break;
		ev = (head ? (QueuedEvent*)head->elem() : NULL);
	}

	if (num_events_processed > 0) {
		Raul::List<Event*> front;
		_events.chop_front(front, num_events_processed, tail);
		dest.append(&front);
	}
}


/** Pre-process a single event */
void
EventSource::_whipped()
{
	Raul::List<Event*>::Node* pb = _prepared_back.get();
	if (!pb)
		return;

	QueuedEvent* const ev = (QueuedEvent*)pb->elem();
	assert(ev);

	assert(!ev->is_prepared());
	ev->pre_process();
	assert(ev->is_prepared());

	assert(_prepared_back.get() == pb);
	_prepared_back = pb->next();

	// If event was blocking, wait for event to being run through the
	// process thread before preparing the next event
	if (ev->is_blocking())
		_blocking_semaphore.wait();
}


} // namespace Ingen

