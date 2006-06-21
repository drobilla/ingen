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

#include "PostProcessor.h"
#include <cassert>
#include <iostream>
#include <pthread.h>
#include "Om.h"
#include "OmApp.h"
#include "Event.h"
#include "util/Queue.h"
#include "Maid.h"


using std::cerr; using std::cout; using std::endl;

namespace Om {

bool PostProcessor::m_process_thread_exit_flag = false;


PostProcessor::PostProcessor(size_t queue_size)
: m_events(queue_size),
  m_thread_exists(false),
  m_semaphore(0)
{
}


PostProcessor::~PostProcessor()
{
	stop();
}


/** Start the process thread.
 */
void
PostProcessor::start() 
{
	cout << "[PostProcessor] Starting." << endl;

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 1500000);

	pthread_create(&m_process_thread, &attr, process_events, this);
	m_thread_exists = true;
}


/** Stop the process thread.
 */
void
PostProcessor::stop()
{
	if (m_thread_exists) {
		m_process_thread_exit_flag = true;
		pthread_cancel(m_process_thread);
		pthread_join(m_process_thread, NULL);
		m_thread_exists = false;
	}
}


/** Signal the PostProcessor to process all pending events.
 */
void
PostProcessor::signal()
{
	m_semaphore.post();
}


void*
PostProcessor::process_events(void* osc_processer)
{
	PostProcessor* me = (PostProcessor*)osc_processer;
	return me->m_process_events();
}


/** OSC message processing thread.
 */
void*
PostProcessor::m_process_events()
{
	Event* ev = NULL;
	
	while (true) {
		m_semaphore.wait();
		
		if (m_process_thread_exit_flag)
			break;
		
		while (!m_events.is_empty()) {
			ev = m_events.pop();
			assert(ev != NULL);
			ev->post_process();
			om->maid()->push(ev);
		}
	}
	
	cout << "[PostProcessor] Exiting post processor thread." << endl;

	return NULL;
}

} // namespace Om
