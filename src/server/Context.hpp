/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

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

#include "ingen/World.hpp"
#include "raul/Atom.hpp"
#include "raul/RingBuffer.hpp"

#include "types.hpp"

namespace Ingen {
namespace Server {

class Engine;
class PortImpl;

/** Graph execution context.
 *
 * This is used to pass whatever information a GraphObject might need to
 * process; such as the current time, a sink for generated events, etc.
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
	enum ID {
		AUDIO,
		MESSAGE
	};

	Context(Engine& engine, size_t event_sink_size, ID id);

	virtual ~Context() {}

	/** Send a notification from this run context. */
	void notify(LV2_URID           key  = 0,
	            FrameTime          time = 0,
	            PortImpl*          port = 0,
	            uint32_t           size = 0,
	            Raul::Atom::TypeID type = 0,
	            const void*        body = NULL);

	/** Emit pending notifications in some other non-realtime thread. */
	void emit_notifications(FrameTime end);

	inline ID id() const { return _id; }

	inline void locate(FrameTime s, SampleCount nframes, SampleCount offset) {
		_start   = s;
		_end     = s + nframes;
		_nframes = nframes;
		_offset  = offset;
	}

	inline void locate(const Context& other) {
		_start   = other._start;
		_end     = other._end;
		_nframes = other._nframes;
		_offset  = other._offset;
	}

	inline Engine&     engine()   const { return _engine; }
	inline FrameTime   start()    const { return _start; }
	inline FrameTime   end()      const { return _end; }
	inline SampleCount nframes()  const { return _nframes; }
	inline SampleCount offset()   const { return _offset; }
	inline bool        realtime() const { return _realtime; }

protected:
	Engine& _engine;  ///< Engine we're running in
	ID      _id;      ///< Fast ID for this context

	Raul::RingBuffer _event_sink; ///< Port updates from process context

	FrameTime   _start;      ///< Start frame of this cycle, timeline relative
	FrameTime   _end;        ///< End frame of this cycle, timeline relative
	SampleCount _nframes;    ///< Length of this cycle in frames
	SampleCount _offset;     ///< Start offset relative to start of driver buffers
	bool        _realtime;   ///< True iff context is hard realtime
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_CONTEXT_HPP

