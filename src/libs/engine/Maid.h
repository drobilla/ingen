/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#ifndef MAID_H
#define MAID_H

#include <boost/utility.hpp>
#include "raul/SRSWQueue.h"
#include "MaidObject.h"


/** Explicitly driven garbage collector.
 *
 * This is how realtime parts of the code can schedule the deletion of
 * objects - push() is realtime safe.
 *
 * cleanup() is meant to be called periodically to free memory, often
 * enough to prevent the queue from overflowing.  This is done by the
 * main thread (in Ingen.cpp) since it has nothing better to do.
 *
 * \ingroup engine
 */
class Maid : boost::noncopyable
{
public:
	Maid(size_t size);
	~Maid();

	inline void push(MaidObject* obj);
	
	void cleanup();
	
private:
	Raul::SRSWQueue<MaidObject*> _objects;
};


/** Push an event to be deleted. Realtime safe.
 */
inline void
Maid::push(MaidObject* obj)
{
	if (obj != NULL)
		_objects.push(obj);
}	


#endif // MAID_H

