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

#ifndef THREAD_H
#define THREAD_H

#include <string>
#include <iostream>
#include <pthread.h>


/** Abstract base class for a thread.
 *
 * Extend this and override the _run method to easily create a thread
 * to perform some task.
 *
 * \ingroup engine
 */
class Thread
{
public:
	Thread() : _pthread_exists(false) {}

	virtual ~Thread() { stop(); }
	
	void set_name(const std::string& name) { _name = name; }
	
	virtual void start() {
		std::cout << "[" << _name << " Thread] Starting." << std::endl;

		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setstacksize(&attr, 1500000);

		pthread_create(&_pthread, &attr, _static_run, this);
		_pthread_exists = true;
	}

	virtual void stop() {
		if (_pthread_exists) {
			pthread_cancel(_pthread);
			pthread_join(_pthread, NULL);
			_pthread_exists = false;
		}
	}

	void set_scheduling(int policy, unsigned int priority) {
		sched_param sp;
		sp.sched_priority = priority;
		int result = pthread_setschedparam(_pthread, SCHED_FIFO, &sp);
		if (!result) {
			std::cout << "[" << _name << "] Set scheduling policy to ";
			switch (policy) {
				case SCHED_FIFO:  std::cout << "SCHED_FIFO";  break;
				case SCHED_RR:    std::cout << "SCHED_RR";    break;
				case SCHED_OTHER: std::cout << "SCHED_OTHER"; break;
				default:          std::cout << "UNKNOWN";     break;
			}
			std::cout << ", priority " << sp.sched_priority << std::endl;
		} else {
			std::cout << "[" << _name << "] Unable to set scheduling policy ("
				<< strerror(result) << ")" << std::endl;
		}
	}


protected:
	virtual void _run() = 0;

private:
	// Prevent copies (undefined)
	Thread(const Thread&);
	Thread& operator=(const Thread&);

	inline static void* _static_run(void* me) {
		Thread* myself = (Thread*)me;
		myself->_run();
		return NULL; // and I
	}

	std::string _name;
	bool        _pthread_exists;
	pthread_t   _pthread;
};


#endif // THREAD_H
