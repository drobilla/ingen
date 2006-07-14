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
	Slave() : _whip(0) {}

	/** Tell the slave to do whatever work it does.  Realtime safe. */
	inline void whip() { _whip.post(); }

protected:
	virtual void _whipped() = 0;

	Semaphore _whip;

private:
	// Prevent copies
	Slave(const Slave&);
	Slave& operator=(const Slave&);

	inline void _run()
	{
		while (true) {
			_whip.wait();
			_whipped();
		}
	}
};


} // namespace Om

#endif // SLAVE_H
