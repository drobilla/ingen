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
: m_front(0),
  m_back(0),
  m_prepared_back(0),
  m_size(size+1),
  m_thread_exists(false),
  m_prepare_thread_exit_flag(false),
  m_semaphore(0)
{
	m_events = (QueuedEvent**)calloc(m_size, sizeof(QueuedEvent*));

	pthread_mutex_init(&m_blocking_mutex, NULL);
	pthread_cond_init(&m_blocking_cond, NULL);

	mlock(m_events, m_size * sizeof(QueuedEvent*));
}


QueuedEventSource::~QueuedEventSource()
{
	stop();
	
	free(m_events);
	pthread_mutex_destroy(&m_blocking_mutex);
	pthread_cond_destroy(&m_blocking_cond);
}


/** Start the prepare thread.
 */
void
QueuedEventSource::start()
{
	if (m_thread_exists) {
		cerr << "[QueuedEventSource] Thread already launched?" << endl;
		return;
	} else {
		cout << "[QueuedEventSource] Launching thread." << endl;
	}

	m_prepare_thread_exit_flag = false;
	
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 1500000);
	
	pthread_create(&m_prepare_thread, &attr, &QueuedEventSource::prepare_loop, this);
	pthread_attr_destroy(&attr);

	m_thread_exists = true;
}


/** Destroy the prepare thread.
 */
void
QueuedEventSource::stop()
{
	if (m_thread_exists) {
		m_prepare_thread_exit_flag = true;
		pthread_cancel(m_prepare_thread);
		pthread_join(m_prepare_thread, NULL);
		m_thread_exists = false;
		cout << "[QueuedEventSource] Stopped thread." << endl;
	}
}


/** Push an unprepared event onto the queue.
 */
void
QueuedEventSource::push(QueuedEvent* const ev)
{
	assert(!ev->is_prepared());

	if (m_events[m_back] != NULL) {
		cerr << "[QueuedEventSource] Error: Queue is full!  Event is lost, please report!" << endl;
		delete ev;
	} else {
		m_events[m_back] = ev;
		m_back = (m_back + 1) % m_size;
		m_semaphore.post();
	}
}


/** Pops the prepared event at the front of the queue, if it exists.
 *
 * This method will only pop events that have been prepared, and are
 * stamped before the time passed.  In other words, it may return NULL
 * even if there are events pending in the queue.  The events returned are
 * actually QueuedEvent*s, but after this they are "normal" events and the
 * engine deals with them just like a realtime in-band event.
 */
Event*
QueuedEventSource::pop_earliest_event_before(const samplecount time)
{
	QueuedEvent* front_event = m_events[m_front];
	
	// Pop
	if (front_event != NULL && front_event->time_stamp() < time && front_event->is_prepared()) {
		m_events[m_front] = NULL;
		m_front = (m_front + 1) % m_size;
		return front_event;
	} else {
		return NULL;
	}
}


// Private //



/** Signal that the blocking event is finished.
 *
 * When this is called preparing will resume.  This will be called by
 * blocking events in their post_process() method.
 */
void
QueuedEventSource::unblock()
{
	/* FIXME: Make this a semaphore, and have events signal at the end of their
	 * execute() methods so the preprocessor can start preparing events immediately
	 * instead of waiting for the postprocessor to get around to finalizing the event? */
	pthread_mutex_lock(&m_blocking_mutex);
	pthread_cond_signal(&m_blocking_cond);
	pthread_mutex_unlock(&m_blocking_mutex);
}


void*
QueuedEventSource::m_prepare_loop()
{
	QueuedEvent* ev = NULL;

	while (true) {
		m_semaphore.wait();
		
		if (m_prepare_thread_exit_flag)
			break; // exit signalled

		ev = m_events[m_prepared_back];
		assert(ev != NULL);
		
		if (ev == NULL) {
			cerr << "[QueuedEventSource] ERROR: Signalled, but event is NULL." << endl;
			continue;
		}

		assert(ev != NULL);
		assert(!ev->is_prepared());

		if (ev->is_blocking())
			pthread_mutex_lock(&m_blocking_mutex);
		
		ev->pre_process();
		
		m_prepared_back = (m_prepared_back+1) % m_size;
		
		// If a blocking event, wait for event to finish passing through
		// the audio cycle before preparing the next event
		if (ev->is_blocking()) {
			pthread_cond_wait(&m_blocking_cond, &m_blocking_mutex);
			pthread_mutex_unlock(&m_blocking_mutex);
		}
	}

	cout << "[QueuedEventSource] Exiting slow event queue thread." << endl;
	return NULL;
}


} // namespace Om

