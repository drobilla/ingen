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

#ifndef QUEUEDEVENTSOURCE_H
#define QUEUEDEVENTSOURCE_H

#include <cstdlib>
#include <pthread.h>
#include "types.hpp"
#include "raul/Semaphore.hpp"
#include "raul/AtomicInt.hpp"
#include "raul/SRSWQueue.hpp"
#include "raul/Slave.hpp"
#include "Event.hpp"
#include "EventSource.hpp"

using Raul::AtomicInt;

namespace Ingen {

class QueuedEvent;
class PostProcessor;


/** Queue of events that need processing before reaching the audio thread.
 *
 * Implemented as a deque (ringbuffer) in a circular array.  Pushing and
 * popping are threadsafe, as long as a single thread pushes and a single
 * thread pops (ie this data structure is threadsafe, but the push and pop
 * methods themselves are not).  Creating an instance of this class spawns
 * a pre-processing thread to prepare queued events.
 *
 * This class is it's own slave. :)
 */
class QueuedEventSource : public EventSource, protected Raul::Slave
{
public:
	QueuedEventSource(size_t queue_size);
	~QueuedEventSource();

	void activate()   { Slave::start(); }
	void deactivate() { Slave::stop(); }

	void process(PostProcessor& dest, ProcessContext& context);

	/** Signal that the blocking event is finished.
	 * When this is called preparing will resume.  This MUST be called by
	 * blocking events in their post_process() method. */
	inline void unblock() { _blocking_semaphore.post(); }

protected:
	void   push_queued(QueuedEvent* const ev);
	Event* pop_earliest_queued_before(const SampleCount time);

	inline bool unprepared_events() { return (_prepared_back.get() != _back.get()); }
	
	virtual void _whipped(); ///< Prepare 1 event

private:
	// Note it's important which functions access which variables for thread safety
	
	// 2-part queue for events that require pre-processing: 
	AtomicInt       _front;         ///< Front of queue
	AtomicInt       _back;          ///< Back of entire queue (index of back event + 1)
	AtomicInt       _prepared_back; ///< Back of prepared events (index of back prepared event + 1)
	const size_t    _size;
	QueuedEvent**   _events;
	Raul::Semaphore _blocking_semaphore;
	Raul::Semaphore _full_semaphore;
};


} // namespace Ingen

#endif // QUEUEDEVENTSOURCE_H
