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

#include <iostream>
#include "EventSink.hpp"
#include "PortImpl.hpp"

using namespace std;

namespace Ingen {
	

/** \a size is not size_t because an event will never be even remotely close
 * to UINT32_MAX in size, so uint32_t saves wasted space on 64-bit.
 */
bool
EventSink::write(uint32_t size, const Event* ev)
{
	if (size > _events.write_space())
		return false;

	_events.write(sizeof(uint32_t), (uint8_t*)&size);
	_events.write(size, (uint8_t*)ev);
		
	return true;
}


/** Read the next event into event_buffer.
 *
 * \a event_buffer can be casted to Event* and virtual methods called.
 */
bool
EventSink::read(uint32_t event_buffer_size, uint8_t* event_buffer)
{
	uint32_t read_size;
	bool success = _events.full_read(sizeof(uint32_t), (uint8_t*)&read_size);
	if (!success)
		return false;

	assert(read_size <= event_buffer_size);

	if (read_size > 0) 
		return _events.full_read(read_size, event_buffer);
	else
		return false;
}


} // namespace Ingen
