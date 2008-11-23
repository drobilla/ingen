/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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
#include <iostream>
#include "QueuedEventSource.hpp"
#include "QueuedEvent.hpp"
#include "PostProcessor.hpp"
#include "ThreadManager.hpp"
#include "ProcessContext.hpp"

using namespace std;

namespace Ingen {


QueuedEventSource::QueuedEventSource(size_t queue_size)
	: _front(0)
	, _back(0)
	, _prepared_back(0)
	, _size(queue_size+1)
	, _blocking_semaphore(0)
	, _full_semaphore(0)
{
	_events = (QueuedEvent**)calloc(_size, sizeof(QueuedEvent*));

	mlock(_events, _size * sizeof(QueuedEvent*));

	Thread::set_context(THREAD_PRE_PROCESS);
	assert(context() == THREAD_PRE_PROCESS);

	set_name("QueuedEventSource");
}


QueuedEventSource::~QueuedEventSource()
{
	Thread::stop();
	
	free(_events);
}


/** Push an unprepared event onto the queue.
 */
void
QueuedEventSource::push_queued(QueuedEvent* const ev)
{
	assert(!ev->is_prepared());

	unsigned back = _back.get();
	bool full = (((_front.get() - back + _size) % _size) == 1);
	while (full) {
		whip();
		cerr << "WARNING: Event queue full.  Waiting..." << endl;
		_full_semaphore.wait();
		back = _back.get();
		full = (((_front.get() - back + _size) % _size) == 1);
	}
		
	assert(_events[back] == NULL);
	_events[back] = ev;
	_back = (back + 1) % _size;
	whip();
}


/** Process all events for a cycle.
 *
 * Executed events will be pushed to @a dest.
 */
void
QueuedEventSource::process(PostProcessor& dest, ProcessContext& context)
{
	assert(ThreadManager::current_thread_id() == THREAD_PROCESS);

	Event* ev = NULL;

	/* Limit the maximum number of queued events to process per cycle.  This
	 * makes the process callback (more) realtime-safe by preventing being
	 * choked by events coming in faster than they can be processed.
	 * FIXME: test this and figure out a good value */
	const unsigned int MAX_QUEUED_EVENTS = context.nframes() / 64;

	unsigned int num_events_processed = 0;
	
	while ((ev = pop_earliest_queued_before(context.end()))) {
		ev->execute(context);
		dest.push(ev);
		if (++num_events_processed > MAX_QUEUED_EVENTS)
			break;
	}
	
	if (_full_semaphore.has_waiter()) {
		const bool full = (((_front.get() - _back.get() + _size) % _size) == 1);
		if (!full)
			_full_semaphore.post();
	}
}


/** Pop the prepared event at the front of the prepare queue, if it exists.
 *
 * This method will only pop events that are prepared and have time stamp
 * less than @a time. It may return NULL even if there are events pending in
 * the queue, if they are unprepared or stamped in the future.
 */
Event*
QueuedEventSource::pop_earliest_queued_before(const SampleCount time)
{
	assert(ThreadManager::current_thread_id() == THREAD_PROCESS);

	const unsigned front = _front.get();
	QueuedEvent* const front_event = _events[front];
	
	// Pop
	if (front_event && front_event->is_prepared() && front_event->time() < time) {
		_events[front] = NULL;
		_front = (front + 1) % _size;
		return front_event;
	} else {
		return NULL;
	}
}


// Private //


/** Pre-process a single event */
void
QueuedEventSource::_whipped()
{
	const unsigned prepared_back = _prepared_back.get();
	QueuedEvent* const ev = _events[prepared_back];
	if (!ev)
		return;
	
	assert(!ev->is_prepared());
	ev->pre_process();
	assert(ev->is_prepared());

	_prepared_back = (prepared_back + 1) % _size;

	// If event was blocking, wait for event to being run through the
	// process thread before preparing the next event
	if (ev->is_blocking())
		_blocking_semaphore.wait();
}


} // namespace Ingen

