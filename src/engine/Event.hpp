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

#ifndef INGEN_ENGINE_EVENT_HPP
#define INGEN_ENGINE_EVENT_HPP

#include <cassert>
#include "raul/SharedPtr.hpp"
#include "raul/Deletable.hpp"
#include "raul/Path.hpp"
#include "types.hpp"

namespace Ingen {

class Engine;
class Request;
class ProcessContext;


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
	virtual void execute(ProcessContext& context);

	/** Perform any actions after execution (ie send replies to commands)
	 * (no realtime requirements). */
	virtual void post_process();

	inline SampleCount time() const { return _time; }

	int error() { return _error; }

protected:
	Event(Engine& engine, SharedPtr<Request> request, FrameTime time)
		: _engine(engine)
		, _request(request)
		, _time(time)
		, _error(0) // success
		, _executed(false)
	{}

	Engine&            _engine;
	SharedPtr<Request> _request;
	FrameTime          _time;
	int                _error;
	bool               _executed;
};


} // namespace Ingen

#endif // INGEN_ENGINE_EVENT_HPP
