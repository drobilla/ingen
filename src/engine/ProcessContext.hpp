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

#ifndef PROCESSCONTEXT_H
#define PROCESSCONTEXT_H

#include "types.hpp"
#include "EventSink.hpp"
#include "Context.hpp"

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
class ProcessContext : public Context
{
public:
	ProcessContext(Engine& engine)
		: Context(engine, AUDIO)
		, _nframes(0)
		, _end(0)
	{}

	void set_time_slice(SampleCount nframes, FrameTime start, FrameTime end) {
		locate(start);
		_nframes = nframes;
		_end = end;
	}

	inline SampleCount nframes()const { return _nframes; }
	inline FrameTime   end()    const { return _end; }

private:
	SampleCount _nframes; ///< Length of this cycle in frames
	FrameTime   _end;     ///< End frame of this cycle, timeline relative
};



} // namespace Ingen

#endif // PROCESSCONTEXT_H

