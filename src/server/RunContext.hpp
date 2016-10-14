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

#ifndef INGEN_ENGINE_RUNCONTEXT_HPP
#define INGEN_ENGINE_RUNCONTEXT_HPP

#include <thread>

#include "ingen/Atom.hpp"
#include "ingen/World.hpp"
#include "raul/RingBuffer.hpp"

#include "types.hpp"

namespace Ingen {
namespace Server {

class Engine;
class PortImpl;
class Task;

/** Graph execution context.
 *
 * This is used to pass whatever information a Node might need to process; such
 * as the current time, a sink for generated events, etc.
 *
 * Note the logical distinction between nframes (jack relative) and start/end
 * (timeline relative).  If transport speed != 1.0, then end-start != nframes
 * (though currently this is never the case, it may be if ingen incorporates
 * tempo and varispeed).
 *
 * \ingroup engine
 */
class RunContext
{
public:
	/** Create a new run context.
	 *
	 * @param engine The engine this context is running within.
	 * @param id The ID of this context.
	 * @param threaded If true, then this context is a worker which will launch
	 * a thread and execute tasks as they become available.
	 */
	RunContext(Engine& engine, unsigned id, bool threaded);

	/** Create a sub-context of `parent`.
	 *
	 * This is used to subdivide process cycles, the sub-context is
	 * lightweight and only serves to pass different time attributes.
	 */
	RunContext(const RunContext& parent);

	virtual ~RunContext();

	/** Return true iff the given port should broadcast its value.
	 *
	 * Whether or not broadcasting is actually done is a per-client property,
	 * this is for use in the audio thread to quickly determine if the
	 * necessary calculations need to be done at all.
	 */
	bool must_notify(const PortImpl* port) const;

	/** Send a notification from this run context.
	 * @return false on failure (ring is full)
	 */
	bool notify(LV2_URID    key  = 0,
	            FrameTime   time = 0,
	            PortImpl*   port = 0,
	            uint32_t    size = 0,
	            LV2_URID    type = 0,
	            const void* body = NULL);

	/** Emit pending notifications in some other non-realtime thread. */
	void emit_notifications(FrameTime end);

	/** Return true iff any notifications are pending. */
	bool pending_notifications() const { return _event_sink->read_space(); }

	/** Return the duration of this cycle in microseconds.
	 *
	 * This is the cycle length in frames (nframes) converted to microseconds,
	 * that is, the amount of real time that this cycle's audio represents.
	 * Note that this is unrelated to the amount of time available to execute a
	 * cycle (other than the fact that it must be processed in significantly
	 * less time to avoid a dropout when running in real time).
	 */
	inline uint64_t duration() const {
		return (uint64_t)_nframes * 1e6 / _rate;
	}

	inline void locate(FrameTime s, SampleCount nframes) {
		_start   = s;
		_end     = s + nframes;
		_nframes = nframes;
	}

	inline void slice(SampleCount offset, SampleCount nframes) {
		_offset  = offset;
		_nframes = nframes;
	}

	inline void set_task(Task* task) {
		_task = task;
	}

	void set_priority(int priority);
	void set_rate(SampleCount rate) { _rate = rate; }

	inline Engine&     engine()   const { return _engine; }
	inline Task*       task()     const { return _task; }
	inline unsigned    id()       const { return _id; }
	inline FrameTime   start()    const { return _start; }
	inline FrameTime   time()     const { return _start + _offset; }
	inline FrameTime   end()      const { return _end; }
	inline SampleCount offset()   const { return _offset; }
	inline SampleCount nframes()  const { return _nframes; }
	inline SampleCount rate()     const { return _rate; }
	inline bool        realtime() const { return _realtime; }

protected:
	const RunContext& operator=(const RunContext& copy) = delete;

	void run();

	Engine&           _engine;      ///< Engine we're running in
	Raul::RingBuffer* _event_sink;  ///< Port updates from process context
	Task*             _task;        ///< Currently executing task
	std::thread*      _thread;      ///< Thread (NULL for main run context)
	unsigned          _id;          ///< Context ID

	FrameTime   _start;      ///< Start frame of this cycle, timeline relative
	FrameTime   _end;        ///< End frame of this cycle, timeline relative
	SampleCount _offset;     ///< Offset into data buffers
	SampleCount _nframes;    ///< Number of frames past offset to process
	SampleCount _rate;       ///< Sample rate in Hz
	bool        _realtime;   ///< True iff context is hard realtime
	bool        _copy;       ///< True iff this is a copy (shared event_sink)
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_RUNCONTEXT_HPP
