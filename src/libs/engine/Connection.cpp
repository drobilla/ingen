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

#include <algorithm>
#include <raul/Maid.hpp>
#include "util.hpp"
#include "Connection.hpp"
#include "Node.hpp"
#include "Port.hpp"
#include "BufferFactory.hpp"
#include "AudioBuffer.hpp"

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
	/*, _must_mix( (src_port->poly() != dst_port->poly())
			|| (src_port->buffer(0)->size() < dst_port->buffer(0)->size()) )*/
	, _must_mix( (src_port->polyphonic() && (! dst_port->polyphonic()))
			|| (src_port->poly() != dst_port->poly() )
			|| (src_port->buffer(0)->size() < dst_port->buffer(0)->size()) )
	, _pending_disconnection(false)
{
	assert(src_port);
	assert(dst_port);
	assert(src_port->type() == dst_port->type());

	/*assert((src_port->parent_node()->poly() == dst_port->parent_node()->poly())
		|| (src_port->parent_node()->poly() == 1 || dst_port->parent_node()->poly() == 1));*/

	if (_must_mix)
		_local_buffer = BufferFactory::create(dst_port->type(), dst_port->buffer(0)->size());

	//cerr << src_port->path() << " -> " << dst_port->path() << " must mix: " << _must_mix << endl;
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
Connection::prepare_poly(uint32_t poly)
{
	_must_mix = (type() == DataType::FLOAT) && (poly > 1) && (
			   (_src_port->poly() != _dst_port->poly())
			|| (_src_port->polyphonic() && !_dst_port->polyphonic())
			|| (_src_port->parent()->polyphonic() && !_dst_port->parent()->polyphonic()) );

	/*cerr << src_port()->path() << " * " << src_port()->poly()
			<< " -> " << dst_port()->path() << " * " << dst_port()->poly()
			<< "\t\tmust mix: " << _must_mix << " at poly " << poly << endl;*/

	if (_must_mix && ! _local_buffer)
		_local_buffer = BufferFactory::create(_dst_port->type(), _dst_port->buffer(0)->size());
}


void
Connection::apply_poly(Raul::Maid& maid, uint32_t poly)
{
	if (poly == 1 && _local_buffer && !_must_mix) {
		maid.push(_local_buffer);
		_local_buffer = NULL;
	}
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

		const AudioBuffer* const src_buffer = (AudioBuffer*)src_port()->buffer(0);
		AudioBuffer*             mix_buf    = (AudioBuffer*)_local_buffer;

		const size_t copy_size = std::min(src_buffer->size(), mix_buf->size());

		/*cerr << "[Connection] Mixing " << src_port()->path() << " * " << src_port()->poly()
			<< " -> " << dst_port()->path() << " * " << dst_port()->poly() << endl;*/

		// Copy src buffer to start of mix buffer
		mix_buf->copy((AudioBuffer*)src_port()->buffer(0), 0, copy_size-1);

		// Write last value of src buffer to remainder of dst buffer, if necessary
		if (copy_size < mix_buf->size())
			mix_buf->set(src_buffer->value_at(copy_size-1), copy_size, mix_buf->size()-1);
	
		// Accumulate the source's voices into local buffer starting at the second
		// voice (buffer is already set to first voice above)
		for (uint32_t j=1; j < src_port()->poly(); ++j) {
			mix_buf->accumulate((AudioBuffer*)src_port()->buffer(j), 0, copy_size-1);
		}
		
		// Find the summed value and write it to the remainder of dst buffer
		if (copy_size < mix_buf->size()) {
			float src_value = src_buffer->value_at(copy_size-1);
			for (uint32_t j=1; j < src_port()->poly(); ++j)
				src_value += ((AudioBuffer*)src_port()->buffer(j))->value_at(copy_size-1);

			mix_buf->set(src_value, copy_size, mix_buf->size()-1);
		}

		// Scale the buffer down.
		if (src_port()->poly() > 1)
			mix_buf->scale(1.0f/(float)src_port()->poly(), 0, _buffer_size-1);
	}
}


} // namespace Ingen

