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

#include "OutputPort.hpp"
#include "AudioBuffer.hpp"
#include "ProcessContext.hpp"

namespace Ingen {


void
OutputPort::pre_process(SampleCount nframes, FrameTime start, FrameTime end)
{
	for (uint32_t i=0; i < _poly; ++i)
		_buffers->at(i)->prepare_write(nframes);
}


void
OutputPort::process(ProcessContext& context, SampleCount nframes, FrameTime start, FrameTime end)
{
	if (_monitor && _type == DataType::FLOAT && _buffer_size == 1) {
		context.event_sink().control_change(this, ((AudioBuffer*)(*_buffers)[0])->value_at(0));
	}
}


void
OutputPort::post_process(SampleCount nframes, FrameTime start, FrameTime end)
{
	for (uint32_t i=0; i < _poly; ++i)
		_buffers->at(i)->prepare_read(nframes);
}


} // namespace Ingen
