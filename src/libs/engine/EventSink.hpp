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

#ifndef EVENTSINK_H
#define EVENTSINK_H

#include <list>
#include <utility>
#include <raul/RingBuffer.hpp>
#include "events/SendPortValueEvent.hpp"
#include "types.hpp"

namespace Ingen {

class Port;
class Engine;
class SendPortValueEvent;


/** Sink for events generated in the audio thread.
 *
 * Implemented as a flat ringbuffer of events, which are constructed directly
 * in the ringbuffer rather than allocated on the heap (in order to make
 * writing realtime safe).
 *
 * \ingroup engine
 */
class EventSink
{
public:
	EventSink(Engine& engine, size_t capacity) : _engine(engine), _events(capacity) {}

	/* FIXME: Figure out variable sized event queues and make this a generic
	 * interface (ie don't add a method for every event type, crap..) */

	bool write(uint32_t size, const Event* ev);

	bool read(uint32_t event_buffer_size, uint8_t* event_buffer);

private:
	Engine& _engine;
	Raul::RingBuffer<uchar> _events;
};



} // namespace Ingen

#endif // EVENTSINK_H

