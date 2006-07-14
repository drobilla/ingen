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

#include "Thread.h"
#include <cassert>
#include <iostream>
#include <pthread.h>

using std::cerr; using std::cout; using std::endl;

namespace Om {


Thread::Thread()
: _pthread_exists(false)
{
}


Thread::~Thread()
{
	stop();
}


/** Start the process thread.
 */
void
Thread::start() 
{
	cout << "[" << _name << " Thread] Starting." << endl;

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 1500000);

	pthread_create(&_pthread, &attr, _static_run, this);
	_pthread_exists = true;
}


/** Stop the process thread.
 */
void
Thread::stop()
{
	if (_pthread_exists) {
		pthread_cancel(_pthread);
		pthread_join(_pthread, NULL);
		_pthread_exists = false;
	}
}


/** Set the scheduling policy for this thread.
 *
 * @param must be one of SCHED_FIFO, SCHED_RR, or SCHED_OTHER.
 */
void
Thread::set_scheduling(int policy, unsigned int priority)
{
	sched_param sp;
	sp.sched_priority = priority;
	int result = pthread_setschedparam(_pthread, SCHED_FIFO, &sp);
	if (!result) {
		cout << "[" << _name << " Thread] Set scheduling policy to ";
		switch (policy) {
		case SCHED_FIFO:  cout << "SCHED_FIFO";  break;
		case SCHED_RR:    cout << "SCHED_RR";    break;
		case SCHED_OTHER: cout << "SCHED_OTHER"; break;
		default:          cout << "UNKNOWN";     break;
		}
		cout << ", priority " << sp.sched_priority << endl;
	} else {
		cout << "[" << _name << " Thread] Unable to set scheduling policy ("
			<< strerror(result) << ")" << endl;
	}
}


void*
Thread::_static_run(void* me)
{
	Thread* myself = (Thread*)me;
	myself->_run();
	// and I
	return NULL;
}

} // namespace Om

