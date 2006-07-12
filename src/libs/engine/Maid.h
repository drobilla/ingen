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

#ifndef MAID_H
#define MAID_H

#include "MaidObject.h"
#include "util/Queue.h"


/** Explicitly driven garbage collector.
 *
 * This is how realtime parts of the code can schedule the deletion of
 * objects - push() is realtime safe.
 *
 * cleanup() is meant to be called periodically to free memory, often
 * enough to prevent the queue from overdflowing.  This is done by the
 * main thread (in OmApp.cpp) since it has nothing better to do.
 *
 * \ingroup engine
 */
class Maid
{
public:
	Maid(size_t size);
	~Maid();

	inline void push(MaidObject* obj);
	
	void cleanup();
	
private:
	// Prevent copies
	Maid(const Maid&);
	Maid& operator=(const Maid&);
	
	Queue<MaidObject*> m_objects;
};


/** Push an event to be deleted. Realtime safe.
 */
inline void
Maid::push(MaidObject* obj)
{
	if (obj != NULL)
		m_objects.push(obj);
}	


#endif // MAID_H

