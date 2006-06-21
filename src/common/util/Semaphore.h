/* This file is part of Om.  Copyright (C) 2005 Dave Robillard.
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

#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <semaphore.h>


/** Trivial wrapper around POSIX semaphores.
 *
 * This was created to provide an alternative debuggable implementation of
 * semaphores based on a cond/mutex pair because semaphore's appeared not to
 * work in GDB.  Turns out sem_wait can fail when run in GDB, and Debian
 * really needs to update it's man pages.
 *
 * This class remains as a pretty wrapper/abstraction that does nothing.
 */
class Semaphore {
public:
	inline Semaphore(unsigned int initial) { sem_init(&m_sem, 0, initial); }
	
	inline ~Semaphore() { sem_destroy(&m_sem); }

	/** Increment (and signal any waiters).
	 * 
	 * Realtime safe.
	 */
	inline void post() { sem_post(&m_sem); }

	/** Wait until count is > 0, then decrement.
	 *
	 * Note that sem_wait always returns 0 in practise.  It returns nonzero
	 * when run in GDB, so the while is necessary to allow debugging.
	 *
	 * Obviously not realtime safe.
	 */
	inline void wait() { while (sem_wait(&m_sem) != 0) ; }
	
	/** Non-blocking version of wait().
	 *
	 * Realtime safe?
	 */
	inline int  try_wait() { return sem_trywait(&m_sem); }
private:
	sem_t m_sem;
};


#endif // SEMAPHORE_H
