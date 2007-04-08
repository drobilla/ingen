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

#include "Connection.h"
#include "util.h"
#include "Node.h"
#include "Port.h"
#include "BufferFactory.h"
#include "AudioBuffer.h"

namespace Ingen {


/** Constructor for a connection from a node's output port.
 *
 * This handles both polyphonic and monophonic nodes, transparently to the 
 * user (InputPort).
 */
Connection::Connection(Port* src_port, Port* dst_port)
	: _src_port(src_port)
	, _dst_port(dst_port)
	, _local_buffer(NULL)
	, _buffer_size(dst_port->buffer_size())
	, _must_mix(src_port->poly() != dst_port->poly())
	, _pending_disconnection(false)
{
	assert(src_port);
	assert(dst_port);
	assert(src_port->type() == dst_port->type());

	assert((src_port->parent_node()->poly() == dst_port->parent_node()->poly())
		|| (src_port->parent_node()->poly() == 1 || dst_port->parent_node()->poly() == 1));

	if (_must_mix)
		_local_buffer = BufferFactory::create(dst_port->type(), dst_port->buffer(0)->size());
}


Connection::~Connection()
{
	delete _local_buffer;
}
	

void
Connection::set_buffer_size(size_t size)
{
	if (_must_mix) {
		assert(_local_buffer);
		delete _local_buffer;

		_local_buffer = BufferFactory::create(_dst_port->type(), _dst_port->buffer(0)->size());
	}
	
	_buffer_size = size;
}


void
Connection::process(SampleCount nframes, FrameTime start, FrameTime end)
{
	// FIXME: nframes parameter not used
	assert(_buffer_size == 1 || _buffer_size == nframes);

	/* Thought:  A poly output port can be connected to multiple mono input
	 * ports, which means this mix down would have to happen many times.
	 * Adding a method to OutputPort that mixes down all it's outputs into
	 * a buffer (if it hasn't been done already this cycle) and returns that
	 * would avoid having to mix multiple times.  Probably not a very common
	 * case, but it would be faster anyway. */
	
	// FIXME: Implement MIDI mixing
	
	if (_must_mix) {
		assert(type() == DataType::FLOAT);

		AudioBuffer* mix_buf = (AudioBuffer*)_local_buffer;

		//cerr << "Mixing " << src_port()->buffer(0)->data()
		//	<< " -> " << _local_buffer->data() << endl;

		mix_buf->copy((AudioBuffer*)src_port()->buffer(0), 0, _buffer_size-1);
	
		// Mix all the source's voices down into local buffer starting at the second
		// voice (buffer is already set to first voice above)
		for (size_t j=1; j < src_port()->poly(); ++j)
			mix_buf->accumulate((AudioBuffer*)src_port()->buffer(j), 0, _buffer_size-1);

		// Scale the buffer down.
		if (src_port()->poly() > 1)
			mix_buf->scale(1.0f/(float)src_port()->poly(), 0, _buffer_size-1);
	}
}


} // namespace Ingen

