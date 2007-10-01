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

#ifndef SENDPORTACTIVITYEVENT_H
#define SENDPORTACTIVITYEVENT_H

#include <string>
#include "Event.hpp"
#include "types.hpp"
using std::string;

namespace Ingen {

class Port;


/** A special event used internally to send port activity notification (e.g.
 * MIDI event activity) from the audio thread.
 *
 * This is created in the audio thread (directly in a ringbuffer, not new'd)
 * for processing in the post processing thread (unlike normal events which
 * are created in the pre-processor thread then run through the audio
 * thread).  This event's job is done entirely in post_process.
 *
 * This only really makes sense for message ports.
 *
 * \ingroup engine
 */
class SendPortActivityEvent : public Event
{
public:
	inline SendPortActivityEvent(Engine&     engine,
	                             SampleCount timestamp,
	                             Port*       port)
		: Event(engine, SharedPtr<Responder>(), timestamp)
		, _port(port)
	{
	}

	inline void operator=(const SendPortActivityEvent& ev) {
		_port = ev._port;
	}
	
	void post_process();

private:
	Port* _port;
};


} // namespace Ingen

#endif // SENDPORTACTIVITYEVENT_H
