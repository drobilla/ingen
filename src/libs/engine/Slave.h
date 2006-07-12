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

#ifndef SLAVE_H
#define SLAVE_H

#include <pthread.h>
#include "util/Semaphore.h"
#include "Thread.h"

namespace Om {


/** Thread driven by (realtime safe) signals.
 *
 * \ingroup engine
 */
class Slave : public Thread
{
public:
	Slave() : _semaphore(0) {}

	inline void signal() { _semaphore.post(); }

protected:
	virtual void _signalled() = 0;

	Semaphore   _semaphore;

private:
	// Prevent copies
	Slave(const Slave&);
	Slave& operator=(const Slave&);

	void _run()
	{
		while (true) {
			_semaphore.wait();
			_signalled();
		}
	}
};


} // namespace Om

#endif // SLAVE_H
