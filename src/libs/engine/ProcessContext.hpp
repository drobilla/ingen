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

#ifndef PROCESSCONTEXT_H
#define PROCESSCONTEXT_H

#include "EventSink.hpp"

namespace Ingen {


/** Context of a process() call.
 *
 * This is used to pass whatever information a GraphObject might need to
 * process in the audio thread, e.g. the available thread pool, sink for
 * events (generated in the audio thread, not user initiated events), etc.
 *
 * Note the distinction between nframes and start/end.  If transport speed
 * != 1.0, end-start != nframes (though currently that is never the case, it
 * may be in the future with sequencerey things).
 *
 * \ingroup engine
 */
class ProcessContext
{
public:
	ProcessContext(Engine& engine) : _event_sink(engine, 1024) {} // FIXME: size?

	void set_time_slice(SampleCount nframes, FrameTime start, FrameTime end) {
		_nframes = nframes;
		_start = start;
		_end = end;
	}

	inline SampleCount       nframes()    const { return _nframes; }
	inline FrameTime         start()      const { return _start; }
	inline FrameTime         end()        const { return _end; }
	inline const EventSink&  event_sink() const { return _event_sink; }
	inline EventSink&        event_sink()       { return _event_sink; }

private:
	SampleCount _nframes;    ///< Number of actual time (Jack) frames this cycle
	FrameTime   _start;      ///< Start frame of this cycle, timeline relative
	FrameTime   _end;        ///< End frame of this cycle, timeline relative
	EventSink   _event_sink; ///< Sink for events generated in the audio thread
};



} // namespace Ingen

#endif // PROCESSCONTEXT_H

