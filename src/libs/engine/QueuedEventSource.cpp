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


QueuedEventSource::QueuedEventSource(size_t queued_size, size_t stamped_size)
: _front(0),
  _back(0),
  _prepared_back(0),
  _size(queued_size+1),
  _blocking_semaphore(0),
  _stamped_queue(stamped_size)
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

	if (_events[_back] != NULL) {
		cerr << "[QueuedEventSource] Error: Queue is full!  Event is lost, please report!" << endl;
		delete ev;
	} else {
		_events[_back] = ev;
		_back = (_back + 1) % _size;
		whip();
	}
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
	const unsigned int MAX_QUEUED_EVENTS = context.nframes() / 100;

	unsigned int num_events_processed = 0;
	
	/* FIXME: Merge these next two loops into one */

	while ((ev = pop_earliest_queued_before(context.end()))) {
		ev->execute(context);
		dest.push(ev);
		if (++num_events_processed > MAX_QUEUED_EVENTS)
			break;
	}
	
	while ((ev = pop_earliest_stamped_before(context.end()))) {
		ev->execute(context);
		dest.push(ev);
		++num_events_processed;
	}

	/*if (num_events_processed > 0)
	    dest.whip();*/
	//else
	//	cerr << "NO PROC: queued: " << unprepared_events() << ", stamped: " << !_stamped_queue.empty() << endl;
}


/** Pops the prepared event at the front of the prepare queue, if it exists.
 *
 * This method will only pop events that have been prepared, and are
 * stamped before the time passed.  In other words, it may return NULL
 * even if there are events pending in the queue.  The events returned are
 * actually QueuedEvents, but after this they are "normal" events and the
 * engine deals with them just like a realtime in-band event.  The engine will
 * not use the timestamps of the returned events in any way, since it is free
 * to execute these non-time-stamped events whenever it wants (at whatever rate
 * it wants).
 */
Event*
QueuedEventSource::pop_earliest_queued_before(const SampleCount time)
{
	assert(ThreadManager::current_thread_id() == THREAD_PROCESS);

	QueuedEvent* const front_event = _events[_front];
	
	// Pop
	if (front_event && front_event->is_prepared() && front_event->time() < time) {
		_events[_front] = NULL;
		_front = (_front + 1) % _size;
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
	QueuedEvent* const ev = _events[_prepared_back];
	assert(ev);
	
	assert(!ev->is_prepared());
	ev->pre_process();
	assert(ev->is_prepared());

	_prepared_back = (_prepared_back+1) % _size;

	// If event was blocking, wait for event to being run through the
	// process thread before preparing the next event
	if (ev->is_blocking())
		_blocking_semaphore.wait();
}


} // namespace Ingen

