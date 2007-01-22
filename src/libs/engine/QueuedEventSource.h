/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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
#include "types.h"
#include "raul/Semaphore.h"
#include "raul/SRSWQueue.h"
#include "raul/Slave.h"
#include "Event.h"
#include "EventSource.h"

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
	QueuedEventSource(size_t queued_size, size_t stamped_size);
	~QueuedEventSource();

	void activate()   { Slave::start(); }
	void deactivate() { Slave::stop(); }

	void process(PostProcessor& dest, SampleCount nframes, FrameTime cycle_start, FrameTime cycle_end);

	void unblock();

protected:
	void push_queued(QueuedEvent* const ev);
	inline void push_stamped(Event* const ev) { _stamped_queue.push(ev); }
	
	Event*        pop_earliest_queued_before(const SampleCount time);
	inline Event* pop_earliest_stamped_before(const SampleCount time);

	bool unprepared_events() { return (_prepared_back != _back); }
	
	virtual void _whipped(); ///< Prepare 1 event

private:
	// Note that it's crucially important which functions access which of these
	// variables, to maintain threadsafeness.
	
	//(FIXME: make this a separate class?)
	// 2-part queue for events that require pre-processing: 
	size_t          _front;         ///< Front of queue
	size_t          _back;          ///< Back of entire queue (1 past index of back element)
	size_t          _prepared_back; ///< Back of prepared section (1 past index of back prepared element)
	const size_t    _size;
	QueuedEvent**   _events;
	Raul::Semaphore _blocking_semaphore;

	/** Queue for timestamped events (no pre-processing). */
	Raul::SRSWQueue<Event*> _stamped_queue;
};


/** Pops the realtime (timestamped, not preprocessed) event off the realtime queue.
 *
 * Engine will use the sample timestamps of returned events directly and execute the
 * event with sample accuracy.  Timestamps in the past will be bumped forward to
 * the beginning of the cycle (offset 0), when eg. skipped cycles occur.
 */
inline Event*
QueuedEventSource::pop_earliest_stamped_before(const SampleCount time)
{
	Event* ret = NULL;

	if (!_stamped_queue.empty() && _stamped_queue.front()->time() < time) {
		ret = _stamped_queue.front();
		_stamped_queue.pop();
	}

	return ret;
}


} // namespace Ingen

#endif // QUEUEDEVENTSOURCE_H
