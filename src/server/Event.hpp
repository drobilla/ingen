/*
  This file is part of Ingen.
  Copyright 2007-2016 David Robillard <http://drobilla.net/>

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

#include "types.hpp"

#include "ingen/Interface.hpp"
#include "ingen/Status.hpp"
#include "ingen/URI.hpp"
#include "ingen/paths.hpp"
#include "raul/Deletable.hpp"
#include "raul/Noncopyable.hpp"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

namespace raul {
class Path;
} // namespace raul

namespace ingen {
namespace server {

class Engine;
class RunContext;
class PreProcessContext;

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
class Event : public raul::Deletable, public raul::Noncopyable
{
public:
	/** Event mode to distinguish normal events from undo events. */
	enum class Mode { NORMAL, UNDO, REDO };

	/** Execution mode for events that block and unblock preprocessing. */
	enum class Execution {
		NORMAL, ///< Normal pipelined execution
		ATOMIC, ///< Block pre-processing until this event is executed
		BLOCK,  ///< Begin atomic block of events
		UNBLOCK ///< Finish atomic executed block of events
	};

	/** Claim position in undo stack before pre-processing (non-realtime). */
	virtual void mark(PreProcessContext&) {}

	/** Pre-process event before execution (non-realtime). */
	virtual bool pre_process(PreProcessContext& ctx) = 0;

	/** Execute this event in the audio thread (realtime). */
	virtual void execute(RunContext& ctx) = 0;

	/** Post-process event after execution (non-realtime). */
	virtual void post_process() = 0;

	/** Write the inverse of this event to `sink`. */
	virtual void undo(Interface& target) {}

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

	/** Return the blocking behaviour of this event (after construction). */
	virtual Execution get_execution() const { return Execution::NORMAL; }

	/** Return undo mode of this event. */
	Mode get_mode() const { return _mode; }

	/** Set the undo mode of this event. */
	void set_mode(Mode mode) { _mode = mode; }

	inline Engine& engine() { return _engine; }

protected:
	Event(Engine&                    engine,
	      std::shared_ptr<Interface> client,
	      int32_t                    id,
	      FrameTime                  time) noexcept
	    : _engine(engine)
	    , _next(nullptr)
	    , _request_client(std::move(client))
	    , _request_id(id)
	    , _time(time)
	    , _status(Status::NOT_PREPARED)
	    , _mode(Mode::NORMAL)
	{}

	/** Constructor for internal events only */
	explicit Event(Engine& engine)
		: _engine(engine)
		, _next(nullptr)
		, _request_id(0)
		, _time(0)
		, _status(Status::NOT_PREPARED)
		, _mode(Mode::NORMAL)
	{}

	inline bool pre_process_done(Status st) {
		_status = st;
		return st == Status::SUCCESS;
	}

	inline bool pre_process_done(Status st, const URI& subject) {
		_err_subject = subject;
		return pre_process_done(st);
	}

	inline bool pre_process_done(Status st, const raul::Path& subject) {
		return pre_process_done(st, path_to_uri(subject));
	}

	/** Respond to the originating client. */
	inline Status respond() {
		if (_request_client && _request_id) {
			_request_client->response(_request_id, _status, _err_subject);
		}
		return _status;
	}

	Engine&                    _engine;
	std::atomic<Event*>        _next;
	std::shared_ptr<Interface> _request_client;
	int32_t                    _request_id;
	FrameTime                  _time;
	Status                     _status;
	std::string                _err_subject;
	Mode                       _mode;
};

} // namespace server
} // namespace ingen

#endif // INGEN_ENGINE_EVENT_HPP
