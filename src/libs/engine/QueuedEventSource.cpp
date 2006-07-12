/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "QueuedEventSource.h"
#include "QueuedEvent.h"
#include <sys/mman.h>
#include <iostream>
using std::cout; using std::cerr; using std::endl;


namespace Om {


QueuedEventSource::QueuedEventSource(size_t size)
: _front(0),
  _back(0),
  _prepared_back(0),
  _size(size+1),
  _blocking_semaphore(0)
{
	_events = (QueuedEvent**)calloc(_size, sizeof(QueuedEvent*));

	mlock(_events, _size * sizeof(QueuedEvent*));
}


QueuedEventSource::~QueuedEventSource()
{
	stop();
	
	free(_events);
}


/** Push an unprepared event onto the queue.
 */
void
QueuedEventSource::push(QueuedEvent* const ev)
{
	assert(!ev->is_prepared());

	if (_events[_back] != NULL) {
		cerr << "[QueuedEventSource] Error: Queue is full!  Event is lost, please report!" << endl;
		delete ev;
	} else {
		_events[_back] = ev;
		_back = (_back + 1) % _size;
		signal();
	}
}


/** Pops the prepared event at the front of the queue, if it exists.
 *
 * This method will only pop events that have been prepared, and are
 * stamped before the time passed.  In other words, it may return NULL
 * even if there are events pending in the queue.  The events returned are
 * actually QueuedEvents, but after this they are "normal" events and the
 * engine deals with them just like a realtime in-band event.
 */
Event*
QueuedEventSource::pop_earliest_before(const samplecount time)
{
	QueuedEvent* const front_event = _events[_front];
	
	// Pop
	if (front_event && front_event->is_prepared() && front_event->time_stamp() < time) {
		_events[_front] = NULL;
		_front = (_front + 1) % _size;
		return front_event;
	} else {
		return NULL;
	}
}


// Private //


/** Signal that the blocking event is finished.
 *
 * When this is called preparing will resume.  This MUST be called by
 * blocking events in their post_process() method.
 */
void
QueuedEventSource::unblock()
{
	_blocking_semaphore.post();
}


void
QueuedEventSource::_signalled()
{
	QueuedEvent* const ev = _events[_prepared_back];
	assert(ev != NULL);

	if (ev == NULL) {
		cerr << "[QueuedEventSource] ERROR: Signalled, but event is NULL." << endl;
		return;
	}

	assert(ev != NULL);
	assert(!ev->is_prepared());

	ev->pre_process();

	_prepared_back = (_prepared_back+1) % _size;

	// If event was blocking, wait for event to being run through the
	// process thread before preparing the next event
	if (ev->is_blocking())
		_blocking_semaphore.wait();
}


} // namespace Om

