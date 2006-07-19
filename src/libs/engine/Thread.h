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
#include <pthread.h>

namespace Ingen {


/* FIXME: This isn't Ingen specific at all.  Move it to util. */


/** Abstract base class for all threads.
 *
 * \ingroup engine
 */
class Thread
{
public:
	Thread();
	virtual ~Thread();

	virtual void start();
	virtual void stop();

	void set_name(const std::string& name) { _name = name; }
	void set_scheduling(int policy, unsigned int priority);

protected:
	virtual void _run() = 0;

	std::string _name;
	pthread_t   _pthread;
	bool        _pthread_exists;

private:
	// Prevent copies
	Thread(const Thread&);
	Thread& operator=(const Thread&);

	static void*  _static_run(void* me);
};


} // namespace Ingen

#endif // THREAD_H
