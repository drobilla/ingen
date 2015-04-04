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

#ifndef INGEN_ENGINE_CONTEXT_HPP
#define INGEN_ENGINE_CONTEXT_HPP

#include "ingen/Atom.hpp"
#include "ingen/World.hpp"
#include "raul/RingBuffer.hpp"

#include "types.hpp"

namespace Ingen {
namespace Server {

class Engine;
class PortImpl;

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
class Context
{
public:
	enum class ID {
		AUDIO,
		MESSAGE
	};

	Context(Engine& engine, ID id);
	Context(const Context& copy);

	virtual ~Context();

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

	inline ID id() const { return _id; }

	inline void locate(FrameTime s, SampleCount nframes) {
		_start   = s;
		_end     = s + nframes;
		_nframes = nframes;
	}

	inline void locate(const Context& other) {
		_start   = other._start;
		_end     = other._end;
		_nframes = other._nframes;
	}

	inline void slice(SampleCount offset, SampleCount nframes) {
		_offset  = offset;
		_nframes = nframes;
	}

	inline Engine&     engine()   const { return _engine; }
	inline FrameTime   start()    const { return _start; }
	inline FrameTime   time()     const { return _start + _offset; }
	inline FrameTime   end()      const { return _end; }
	inline SampleCount offset()   const { return _offset; }
	inline SampleCount nframes()  const { return _nframes; }
	inline bool        realtime() const { return _realtime; }

protected:
	const Context& operator=(const Context& copy) = delete;

	Engine& _engine;  ///< Engine we're running in
	ID      _id;      ///< Fast ID for this context

	Raul::RingBuffer* _event_sink; ///< Port updates from process context

	FrameTime   _start;      ///< Start frame of this cycle, timeline relative
	FrameTime   _end;        ///< End frame of this cycle, timeline relative
	SampleCount _offset;     ///< Offset into data buffers
	SampleCount _nframes;    ///< Number of frames past offset to process
	bool        _realtime;   ///< True iff context is hard realtime
	bool        _copy;       ///< True iff this is a copy (shared event_sink)
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_CONTEXT_HPP
