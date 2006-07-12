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

#ifndef QUEUEDEVENTSOURCE_H
#define QUEUEDEVENTSOURCE_H

#include <cstdlib>
#include <pthread.h>
#include "types.h"
#include "util/Semaphore.h"
#include "Slave.h"
#include "EventSource.h"

namespace Om {

class Event;
class QueuedEvent;


/** Queue of events that need processing before reaching the audio thread.
 *
 * Implemented as a deque (ringbuffer) in a circular array.  Pushing and
 * popping are threadsafe, as long as a single thread pushes and a single
 * thread pops (ie this data structure is threadsafe, but the push and pop
 * methods themselves are not).
 *
 * This class is it's own slave. :)
 */
class QueuedEventSource : public EventSource, protected Slave
{
public:
	QueuedEventSource(size_t size);
	~QueuedEventSource();

	void start() { Thread::start(); }
	void stop()  { Thread::stop(); }

	Event* pop_earliest_before(const samplecount time);

	void unblock();

protected:
	void push(QueuedEvent* const ev);
	bool unprepared_events() { return (_prepared_back != _back); }
	
	virtual void _signalled(); ///< Prepare 1 event

private:
	// Prevent copies (undefined)
	QueuedEventSource(const QueuedEventSource&);
	QueuedEventSource& operator=(const QueuedEventSource&);
	
	// Note that it's crucially important which functions access which of these
	// variables, to maintain threadsafeness.
	
	size_t         _front;         ///< Front of queue
	size_t         _back;          ///< Back of entire queue (1 past index of back element)
	size_t         _prepared_back; ///< Back of prepared section (1 past index of back prepared element)
	const size_t   _size;
	QueuedEvent**  _events;
	Semaphore      _blocking_semaphore;
};


} // namespace Om

#endif // QUEUEDEVENTSOURCE_H
