/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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

#ifndef INGEN_ENGINE_CONTEXT_HPP
#define INGEN_ENGINE_CONTEXT_HPP

#include "EventSink.hpp"
#include "tuning.hpp"

namespace Ingen {

class Engine;

class Context
{
public:
	enum ID {
		AUDIO,
		MESSAGE
	};

	Context(Engine& engine, ID id)
		: _id(id)
		, _engine(engine)
		, _event_sink(engine, event_queue_size)
		, _start(0)
		, _nframes(0)
		, _end(0)
		, _offset(0)
		, _realtime(true)
	{}

	virtual ~Context() {}

	ID id() const { return _id; }

	void locate(FrameTime s, SampleCount o=0) { _start = s; _offset=o; }

	inline Engine&     engine()   const { return _engine; }
	inline FrameTime   start()    const { return _start; }
	inline SampleCount nframes()  const { return _nframes; }
	inline FrameTime   end()      const { return _end; }
	inline SampleCount offset()   const { return _offset; }
	inline bool        realtime() const { return _realtime; }

	inline const EventSink&  event_sink() const { return _event_sink; }
	inline EventSink&        event_sink()       { return _event_sink; }

protected:
	ID      _id;      ///< Fast ID for this context
	Engine& _engine;  ///< Engine we're running in

	EventSink   _event_sink; ///< Sink for events generated in a realtime context
	FrameTime   _start;      ///< Start frame of this cycle, timeline relative
	SampleCount _nframes;    ///< Length of this cycle in frames
	FrameTime   _end;        ///< End frame of this cycle, timeline relative
	SampleCount _offset;     ///< Start offset relative to start of driver buffers
	bool        _realtime;   ///< True iff context is hard realtime
};


} // namespace Ingen

#endif // INGEN_ENGINE_CONTEXT_HPP

