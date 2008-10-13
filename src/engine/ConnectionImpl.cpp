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
#include "ConnectionImpl.hpp"
#include "NodeImpl.hpp"
#include "PortImpl.hpp"
#include "AudioBuffer.hpp"
#include "ProcessContext.hpp"

/*#include <iostream>
using namespace std;*/

namespace Ingen {


/** Constructor for a connection from a node's output port.
 *
 * This handles both polyphonic and monophonic nodes, transparently to the 
 * user (InputPort).
 */
ConnectionImpl::ConnectionImpl(PortImpl* src_port, PortImpl* dst_port)
	: _mode(DIRECT)
	, _src_port(src_port)
	, _dst_port(dst_port)
	, _local_buffer(NULL)
	, _buffer_size(dst_port->buffer_size())
	, _pending_disconnection(false)
{
	assert(src_port);
	assert(dst_port);
	assert(src_port != dst_port);
	assert(src_port->path() != dst_port->path());
	assert(src_port->type() == dst_port->type()
			|| ( (src_port->type() == DataType::CONTROL || src_port->type() == DataType::AUDIO)
				&& (dst_port->type() == DataType::CONTROL || dst_port->type() == DataType::AUDIO) ));

	/*assert((src_port->parent_node()->poly() == dst_port->parent_node()->poly())
		|| (src_port->parent_node()->poly() == 1 || dst_port->parent_node()->poly() == 1));*/

	set_mode();
	if (need_buffer())
		_local_buffer = Buffer::create(dst_port->type(), _buffer_size);

	/* FIXME: 1->1 connections with a destination with fixed buffers copies unecessarily */
	//cerr << src_port->path() << " -> " << dst_port->path() << " must mix: " << _must_mix << endl;
}


ConnectionImpl::~ConnectionImpl()
{
	delete _local_buffer;
}
	

void
ConnectionImpl::set_mode()
{
	if (must_copy())
		_mode = COPY;
	else if (must_mix())
		_mode = MIX;
	else if (must_extend())
		_mode = EXTEND;
	
	if (type() == DataType::EVENT)
		_mode = DIRECT; // FIXME: kludge
}


void
ConnectionImpl::set_buffer_size(size_t size)
{
	if (_mode == MIX || _mode == EXTEND) {
		assert(_local_buffer);
		delete _local_buffer;

		_local_buffer = Buffer::create(_dst_port->type(), _dst_port->buffer(0)->size());
	}
	
	_buffer_size = size;
}


bool
ConnectionImpl::must_copy() const
{
	return (_dst_port->fixed_buffers() && (_src_port->poly() == _dst_port->poly()));
}


bool
ConnectionImpl::must_mix() const
{
	return (   (_src_port->polyphonic() && !_dst_port->polyphonic())
	        || (_src_port->parent()->polyphonic() && !_dst_port->parent()->polyphonic())
	        || (_dst_port->fixed_buffers()) );
}


bool
ConnectionImpl::must_extend() const
{
	return (_src_port->buffer_size() != _dst_port->buffer_size());
}


void
ConnectionImpl::prepare_poly(uint32_t poly)
{
	_src_port->prepare_poly(poly);

	/*cerr << "CONNECTION PREPARE: " << src_port()->path() << " * " << src_port()->poly()
			<< " -> " << dst_port()->path() << " * " << dst_port()->poly()
			<< "\t\tmust mix: " << must_mix() << " at poly " << poly << endl;*/

	if (need_buffer() && !_local_buffer)
		_local_buffer = Buffer::create(_dst_port->type(), _dst_port->buffer(0)->size());
}


void
ConnectionImpl::apply_poly(Raul::Maid& maid, uint32_t poly)
{
	_src_port->apply_poly(maid, poly);
	set_mode();
	
	/*cerr << "CONNECTION APPLY: " << src_port()->path() << " * " << src_port()->poly()
			<< " -> " << dst_port()->path() << " * " << dst_port()->poly()
			<< "\t\tmust mix: " << must_mix() << ", extend: " << must_extend()
			<< ", poly " << poly << endl;*/

	// Recycle buffer if it's no longer needed
	if (_local_buffer && !need_buffer()) {
		maid.push(_local_buffer);
		_local_buffer = NULL;
	}
}


void
ConnectionImpl::process(ProcessContext& context)
{
	// FIXME: nframes parameter not used
	assert(_buffer_size == 1 || _buffer_size == context.nframes());

	/* Thought:  A poly output port can be connected to multiple mono input
	 * ports, which means this mix down would have to happen many times.
	 * Adding a method to OutputPort that mixes down all it's outputs into
	 * a buffer (if it hasn't been done already this cycle) and returns that
	 * would avoid having to mix multiple times.  Probably not a very common
	 * case, but it would be faster anyway. */

	/*cerr << src_port()->path() << " * " << src_port()->poly()
			<< " -> " << dst_port()->path() << " * " << dst_port()->poly()
			<< "\t\tmode: " << (int)_mode << endl;*/
	
	if (_mode == COPY) {
		assert(src_port()->poly() == dst_port()->poly());
		const size_t copy_size = std::min(src_port()->buffer_size(), dst_port()->buffer_size());
		for (uint32_t i=0; i < src_port()->poly(); ++i) {
			dst_port()->buffer(i)->copy(src_port()->buffer(i), 0, copy_size);
		}
	} else if (_mode == MIX) {
		assert(type() == DataType::AUDIO || type() == DataType::CONTROL);

		const AudioBuffer* const src_buffer = (AudioBuffer*)src_port()->buffer(0);
		AudioBuffer*             mix_buf    = (AudioBuffer*)_local_buffer;
		
		assert(mix_buf);

		const size_t copy_size = std::min(src_buffer->size(), mix_buf->size());

		// Copy src buffer to start of mix buffer
		mix_buf->copy((AudioBuffer*)src_port()->buffer(0), 0, copy_size-1);

		// Write last value of src buffer to remainder of dst buffer, if necessary
		if (copy_size < mix_buf->size())
			mix_buf->set_block(src_buffer->value_at(copy_size-1), copy_size, mix_buf->size()-1);
	
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

			mix_buf->set_block(src_value, copy_size, mix_buf->size()-1);
		}

		// Scale the buffer down.
		if (src_port()->poly() > 1)
			mix_buf->scale(1.0f/(float)src_port()->poly(), 0, mix_buf->size()-1);
	
	} else if (_mode == EXTEND) {
		assert(type() == DataType::AUDIO || type() == DataType::CONTROL);
		assert(src_port()->poly() == 1 || src_port()->poly() == dst_port()->poly());

		const uint32_t poly      = dst_port()->poly();
		const uint32_t copy_size = std::min(src_port()->buffer(0)->size(),
		                                    dst_port()->buffer(0)->size());
		
		for (uint32_t i = 0; i < poly; ++i) {
			uint32_t     src_voice = std::min(i, src_port()->poly() - 1);
			AudioBuffer* src_buf   = (AudioBuffer*)src_port()->buffer(src_voice);
			AudioBuffer* dst_buf   = (AudioBuffer*)dst_port()->buffer(i);

			// Copy src to start of dst
			dst_buf->copy(src_buf, 0, copy_size-1);
		
			// Write last value of src buffer to remainder of dst buffer, if necessary
			if (copy_size < dst_buf->size())
				dst_buf->set_block(src_buf->value_at(copy_size - 1), copy_size, dst_buf->size() - 1);
		}
	}

}


} // namespace Ingen

