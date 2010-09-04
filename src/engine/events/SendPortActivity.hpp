/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#ifndef INGEN_EVENTS_SENDPORTACTIVITY_HPP
#define INGEN_EVENTS_SENDPORTACTIVITY_HPP

#include "Event.hpp"
#include "types.hpp"

namespace Ingen {

class PortImpl;

namespace Events {


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
class SendPortActivity : public Event
{
public:
	inline SendPortActivity(Engine&     engine,
	                        SampleCount timestamp,
	                        PortImpl*   port)
		: Event(engine, SharedPtr<Request>(), timestamp)
		, _port(port)
	{
	}

	inline SendPortActivity& operator=(const SendPortActivity& ev) {
		_port = ev._port;
		return *this;
	}

	void post_process();

private:
	PortImpl* _port;
};


} // namespace Ingen
} // namespace Events

#endif // INGEN_EVENTS_SENDPORTACTIVITY_HPP
