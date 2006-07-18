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

#ifndef EVENT_H
#define EVENT_H

#include <cassert>
#include "util/CountedPtr.h"
#include "types.h"
#include "MaidObject.h"
#include "Responder.h"

namespace Om {	



/** Base class for all events (both realtime and QueuedEvent).
 *
 * This is for time-critical events like note ons.  There is no non-realtime
 * pre-execute method as in QueuedEvent's, any lookups etc need to be done in the
 * realtime execute() method.
 *
 * QueuedEvent extends this class with a pre_process() method for any work that needs
 * to be done before processing in the realtime audio thread.
 *
 * \ingroup engine
 */
class Event : public MaidObject
{
public:
	virtual ~Event() {}

	/** Execute this event in the audio thread (MUST be realtime safe). */
	virtual void execute(SampleCount offset) { assert(!_executed); _executed = true; }
	
	/** Perform any actions after execution (ie send replies to commands)
	 * (no realtime requirements). */
	virtual void post_process() {}
	
	inline SampleCount time_stamp() { return _time_stamp; }
		
protected:
	// Prevent copies
	Event(const Event&);
	Event& operator=(const Event&);

	Event(CountedPtr<Responder> responder, SampleCount timestamp)
	: _responder(responder)
	, _time_stamp(timestamp)
	, _executed(false)
	{}
	
	CountedPtr<Responder>  _responder;
	SampleCount            _time_stamp;
	bool                   _executed;
};


} // namespace Om

#endif // EVENT_H
