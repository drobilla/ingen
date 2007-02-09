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

#ifndef EVENT_H
#define EVENT_H

#include <cassert>
#include "raul/SharedPtr.h"
#include "types.h"
#include <raul/Deletable.h>
#include "Responder.h"
#include "ThreadManager.h"

namespace Raul { class Path; }
using Raul::Path;

namespace Ingen {	

class Engine;


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
class Event : public Raul::Deletable
{
public:
	virtual ~Event() {}

	/** Execute this event in the audio thread (MUST be realtime safe). */
	virtual void execute(SampleCount nframes, FrameTime start, FrameTime end)
	{
		assert(ThreadManager::current_thread_id() == THREAD_PROCESS);
		assert(!_executed);
		assert(_time <= end);

		// Missed the event, jitter, damnit.
		if (_time < start)
			_time = start;

		_executed = true;
	}
	
	/** Perform any actions after execution (ie send replies to commands)
	 * (no realtime requirements). */
	virtual void post_process()
	{
		assert(ThreadManager::current_thread_id() == THREAD_POST_PROCESS);
	}
	
	inline SampleCount time() { return _time; }
		
protected:
	Event(Engine& engine, SharedPtr<Responder> responder, FrameTime time)
	: _engine(engine)
	, _responder(responder)
	, _time(time)
	, _executed(false)
	{}
	
	Engine&              _engine;
	SharedPtr<Responder> _responder;
	FrameTime            _time;
	bool                 _executed;
};


} // namespace Ingen

#endif // EVENT_H
