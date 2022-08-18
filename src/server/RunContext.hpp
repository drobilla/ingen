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

#include "types.hpp"

#include "lv2/urid/urid.h"
#include "raul/RingBuffer.hpp"

#include <cstdint>
#include <memory>
#include <thread>

namespace ingen {
namespace server {

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
	 * @param event_sink Sink for notification events (peaks etc)
	 * @param id The ID of this context.
	 * @param threaded If true, then this context is a worker which will launch
	 * a thread and execute tasks as they become available.
	 */
	RunContext(Engine&           engine,
	           raul::RingBuffer* event_sink,
	           unsigned          id,
	           bool              threaded);

	/** Create a sub-context of `parent`.
	 *
	 * This is used to subdivide process cycles, the sub-context is
	 * lightweight and only serves to pass different time attributes.
	 */
	RunContext(const RunContext& copy);

	RunContext& operator=(const RunContext&) = delete;
	RunContext& operator=(RunContext&&) = delete;

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
	            PortImpl*   port = nullptr,
	            uint32_t    size = 0,
	            LV2_URID    type = 0,
	            const void* body = nullptr);

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
	uint64_t duration() const {
		return static_cast<uint64_t>(_nframes) * 1e6 / _rate;
	}

	void locate(FrameTime s, SampleCount nframes) {
		_start   = s;
		_end     = s + nframes;
		_nframes = nframes;
	}

	void slice(SampleCount offset, SampleCount nframes) {
		_offset  = offset;
		_nframes = nframes;
	}

	/** Claim a parallel task, and signal others that work is available. */
	void claim_task(Task* task);

	/** Steal a task from some other context if possible. */
	Task* steal_task() const;

	void set_priority(int priority);
	void set_rate(SampleCount rate) { _rate = rate; }

    void join();

	Engine&     engine()   const { return _engine; }
	Task*       task()     const { return _task; }
	unsigned    id()       const { return _id; }
	FrameTime   start()    const { return _start; }
	FrameTime   time()     const { return _start + _offset; }
	FrameTime   end()      const { return _end; }
	SampleCount offset()   const { return _offset; }
	SampleCount nframes()  const { return _nframes; }
	SampleCount rate()     const { return _rate; }
	bool        realtime() const { return _realtime; }

protected:
	void run();

	Engine&                      _engine;        ///< Engine we're running in
	raul::RingBuffer*            _event_sink;    ///< Updates from notify()
	Task*                        _task{nullptr}; ///< Currently executing task
	std::unique_ptr<std::thread> _thread;        ///< Thread (or null for main)
	unsigned                     _id;            ///< Context ID

	FrameTime   _start{0};       ///< Start frame of this cycle (timeline)
	FrameTime   _end{0};         ///< End frame of this cycle (timeline)
	SampleCount _offset{0};      ///< Offset into data buffers
	SampleCount _nframes{0};     ///< Number of frames past offset to process
	SampleCount _rate{0};        ///< Sample rate in Hz
	bool        _realtime{true}; ///< True iff context is hard realtime
};

} // namespace server
} // namespace ingen

#endif // INGEN_ENGINE_RUNCONTEXT_HPP
