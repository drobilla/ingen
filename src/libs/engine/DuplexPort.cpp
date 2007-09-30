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

#include <iostream>
#include <cstdlib>
#include <cassert>
#include "util.hpp"
#include "DuplexPort.hpp"
#include "Connection.hpp"
#include "OutputPort.hpp"
#include "Node.hpp"

using namespace std;

namespace Ingen {


DuplexPort::DuplexPort(Node* parent, const string& name, uint32_t index, uint32_t poly, DataType type, size_t buffer_size, bool is_output)
	: Port(parent, name, index, poly, type, buffer_size)
	, InputPort(parent, name, index, poly, type, buffer_size)
	, OutputPort(parent, name, index, poly, type, buffer_size)
	, _is_output(is_output)
{
	assert(Port::_parent == parent);
}


void
DuplexPort::pre_process(SampleCount nframes, FrameTime start, FrameTime end)
{
	// Think about it...

//	if (_is_output) {
		InputPort::pre_process(nframes, start, end);
//	} else {
		//for (uint32_t i=0; i < _poly; ++i)
		//	_buffers->at(i)->rewind();
//		OutputPort::pre_process(nframes, start, end);
//	}
}

void
DuplexPort::process(ProcessContext& context, SampleCount nframes, FrameTime start, FrameTime end)
{
}


void
DuplexPort::post_process(SampleCount nframes, FrameTime start, FrameTime end)
{
	// Think about it...
	
//	if (_is_output)
//		InputPort::pre_process(nframes, start, end);
//else
		OutputPort::pre_process(nframes, start, end);
}


} // namespace Ingen

