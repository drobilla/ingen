/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
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

#include "raul/AtomicPtr.hpp"
#include "raul/Deletable.hpp"
#include "raul/Noncopyable.hpp"
#include "raul/Path.hpp"
#include "raul/SharedPtr.hpp"

#include "ingen/Status.hpp"

#include "types.hpp"

namespace Ingen {

class ClientInterface;

namespace Server {

class Engine;
class ProcessContext;

/** An event (command) to perform some action on Ingen.
 *
 * Virtually all operations on Ingen are implemented as events.  An event has
 * three distinct execution phases:
 *
 * 1) Pre-process: In a non-realtime thread, prepare event for execution
 * 2) Execute: In the audio thread, execute (apply) event
 * 3) Post-process: In a non-realtime thread, finalize event
 *    (e.g. clean up and send replies)
 *
 * \ingroup engine
 */
class Event : public Raul::Deletable, public Raul::Noncopyable
{
public:
	virtual ~Event() {}

	/** Pre-process event before execution (non-realtime). */
	virtual void pre_process();

	/** Execute this event in the audio thread (realtime). */
	virtual void execute(ProcessContext& context);

	/** Post-process event after execution (non-realtime). */
	virtual void post_process();

	/** Return true iff this event has been pre-processed. */
	inline bool is_prepared() const { return _pre_processed; }

	/** Return the time stamp of this event. */
	inline SampleCount time() const { return _time; }

	/** Get the next event to be processed after this one. */
	Event* next() const { return _next.get(); }

	/** Set the next event to be processed after this one. */
	void next(Event* ev) { _next = ev; }

	/** Return the status (success or error code) of this event. */
	Status status() const { return _status; }

	/** Respond to the originating client. */
	void respond(Status status);

protected:
	Event(Engine& engine, ClientInterface* client, int32_t id, FrameTime time)
		: _engine(engine)
		, _request_client(client)
		, _request_id(id)
		, _time(time)
		, _status(SUCCESS)
		, _pre_processed(false)
		, _executed(false)
	{}

	/** Constructor for internal events only */
	explicit Event(Engine& engine)
		: _engine(engine)
		, _request_client(NULL)
		, _request_id(-1)
		, _time(0)
		, _status(SUCCESS)
		, _pre_processed(false)
		, _executed(false)
	{}

	Engine&                _engine;
	Raul::AtomicPtr<Event> _next;
	ClientInterface*       _request_client;
	int32_t                _request_id;
	FrameTime              _time;
	Status                 _status;
	bool                   _pre_processed;
	bool                   _executed;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_EVENT_HPP
