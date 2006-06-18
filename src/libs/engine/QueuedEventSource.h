/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
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
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef QUEUEDEVENTSOURCE_H
#define QUEUEDEVENTSOURCE_H

#include <cstdlib>
#include <pthread.h>
#include "types.h"
#include "util/Semaphore.h"
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
 */
class QueuedEventSource : public EventSource
{
public:
	QueuedEventSource(size_t size);
	~QueuedEventSource();

	Event* pop_earliest_event_before(const samplecount time);

	void unblock();

	void start();
	void stop();

protected:
	void push(QueuedEvent* const ev);

private:
	// Prevent copies (undefined)
	QueuedEventSource(const QueuedEventSource&);
	QueuedEventSource& operator=(const QueuedEventSource&);
	
	// Note that it's crucially important which functions access which of these
	// variables, to maintain threadsafeness.
	
	size_t      m_front;         ///< Front of queue
	size_t      m_back;          ///< Back of entire queue (1 past index of back element)
	size_t      m_prepared_back; ///< Back of prepared section (1 past index of back prepared element)
	const size_t m_size;
	QueuedEvent**  m_events;
	
	bool            m_thread_exists;
	bool            m_prepare_thread_exit_flag;
	pthread_t       m_prepare_thread;
	Semaphore       m_semaphore; ///< Counting semaphor for driving prepare thread
	pthread_mutex_t m_blocking_mutex;
	pthread_cond_t  m_blocking_cond;

	static void* prepare_loop(void* q) { return ((QueuedEventSource*)q)->m_prepare_loop(); }
	void* m_prepare_loop();
};


} // namespace Om

#endif // QUEUEDEVENTSOURCE_H
