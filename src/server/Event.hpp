/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_ENGINE_EVENT_HPP
#define INGEN_ENGINE_EVENT_HPP

#include <atomic>

#include "raul/Deletable.hpp"
#include "raul/Noncopyable.hpp"
#include "raul/Path.hpp"

#include "ingen/Interface.hpp"
#include "ingen/Node.hpp"
#include "ingen/Status.hpp"
#include "ingen/types.hpp"

#include "types.hpp"

namespace Ingen {
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
	virtual bool pre_process() = 0;

	/** Execute this event in the audio thread (realtime). */
	virtual void execute(ProcessContext& context) = 0;

	/** Post-process event after execution (non-realtime). */
	virtual void post_process() = 0;

	/** Return true iff this event has been pre-processed. */
	inline bool is_prepared() const { return _status != Status::NOT_PREPARED; }

	/** Return the time stamp of this event. */
	inline SampleCount time() const { return _time; }

	/** Set the time stamp of this event. */
	inline void set_time(SampleCount time) { _time = time; }

	/** Get the next event to be processed after this one. */
	Event* next() const { return _next.load(); }

	/** Set the next event to be processed after this one. */
	void next(Event* ev) { _next = ev; }

	/** Return the status (success or error code) of this event. */
	Status status() const { return _status; }

protected:
	Event(Engine& engine, SPtr<Interface> client, int32_t id, FrameTime time)
		: _engine(engine)
		, _next(NULL)
		, _request_client(client)
		, _request_id(id)
		, _time(time)
		, _status(Status::NOT_PREPARED)
	{}

	/** Constructor for internal events only */
	explicit Event(Engine& engine)
		: _engine(engine)
		, _next(NULL)
		, _request_id(0)
		, _time(0)
		, _status(Status::NOT_PREPARED)
	{}

	inline bool pre_process_done(Status st) {
		_status = st;
		return st == Status::SUCCESS;
	}

	inline bool pre_process_done(Status st, const Raul::URI& subject) {
		_err_subject = subject;
		return pre_process_done(st);
	}

	inline bool pre_process_done(Status st, const Raul::Path& subject) {
		return pre_process_done(st, Node::path_to_uri(subject));
	}

	/** Respond to the originating client. */
	inline Status respond() {
		if (_request_client && _request_id) {
			_request_client->response(_request_id, _status, _err_subject);
		}
		return _status;
	}

	Engine&             _engine;
	std::atomic<Event*> _next;
	SPtr<Interface>     _request_client;
	int32_t             _request_id;
	FrameTime           _time;
	Status              _status;
	std::string         _err_subject;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_EVENT_HPP
