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

#ifndef POSTPROCESSOR_H
#define POSTPROCESSOR_H

#include <pthread.h>
#include "types.h"
#include "util/Queue.h"
#include "util/Semaphore.h"

namespace Om {

class Event;


/** Processor for Events after leaving the audio thread.
 *
 * The audio thread pushes events to this when it is done with them (which
 * is realtime-safe), which signals the processing thread through a semaphore
 * to handle the event and pass it on to the Maid.
 *
 * \ingroup engine
 */
class PostProcessor
{
public:
	PostProcessor(size_t queue_size);
	~PostProcessor();

	void start();
	void stop();

	inline void push(Event* const ev);
	void        signal();

private:
	// Prevent copies
	PostProcessor(const PostProcessor&);
	PostProcessor& operator=(const PostProcessor&);

	Queue<Event*> m_events;

	static void* process_events(void* me);
	void*        m_process_events();

	pthread_t   m_process_thread;
	bool        m_thread_exists;
	static bool m_process_thread_exit_flag;
	Semaphore   m_semaphore;
};


/** Push an event on to the process queue, realtime-safe, not thread-safe.
 */
inline void
PostProcessor::push(Event* const ev)
{
	m_events.push(ev);
}


} // namespace Om

#endif // POSTPROCESSOR_H
