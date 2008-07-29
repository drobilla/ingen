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
#include <cassert>
#include <stdlib.h>
#include "AudioBuffer.hpp"

using namespace std;

/* TODO: Be sure these functions are vectorized by GCC when it's vectorizer
 * stops sucking.  Probably a good idea to inline them as well */

namespace Ingen {


AudioBuffer::AudioBuffer(size_t size)
	: Buffer((size == 1) ? DataType::CONTROL : DataType::AUDIO, size)
	, _data(NULL)
	, _local_data(NULL)
	, _size(size)
	, _filled_size(0)
	, _state(OK)
	, _set_value(0)
	, _set_time(0)
{
	assert(_size > 0);
	allocate();
	assert(data());
}


void
AudioBuffer::resize(size_t size)
{
	_size = size;

	Sample* const old_data = _data;
	
	const bool using_local_data = (_data == _local_data);

	deallocate();

	const int ret = posix_memalign((void**)&_local_data, 16, _size * sizeof(Sample));
	if (ret != 0) {
		cerr << "[Buffer] Failed to allocate buffer.  Aborting." << endl;
		exit(EXIT_FAILURE);
	}
		
	assert(ret == 0);
	assert(_local_data);
	
	if (using_local_data)
		_data = _local_data;
	else
		_data = old_data;

	set_block(0, 0, _size-1);
}


/** Allocate and use a locally managed buffer (data).
 */
void
AudioBuffer::allocate()
{
	assert(!_joined_buf);
	assert(_local_data == NULL);
	assert(_size > 0);

	const int ret = posix_memalign((void**)&_local_data, 16, _size * sizeof(Sample));
	if (ret != 0) {
		cerr << "[Buffer] Failed to allocate buffer.  Aborting." << endl;
		exit(EXIT_FAILURE);
	}
		
	assert(ret == 0);
	assert(_local_data);

	_data = _local_data;

	set_block(0, 0, _size-1);
}


/** Free locally allocated buffer.
 */
void
AudioBuffer::deallocate()
{
	assert(!_joined_buf);
	free(_local_data);
	_local_data = NULL;
	_data = NULL;
}


/** Empty (ie zero) the buffer.
 */
void
AudioBuffer::clear()
{
	set_block(0, 0, _size-1);
	_state = OK;
	_filled_size = 0;
}


/** Set value of buffer to @a val after @a start_sample.
 *
 * The Buffer will handle setting the intial portion of the buffer to the
 * value on the next cycle automatically (if @a start_sample is > 0), as
 * long as pre_process() is called every cycle.
 */
void
AudioBuffer::set_value(Sample val, FrameTime cycle_start, FrameTime time)
{
	FrameTime offset = time - cycle_start;
	assert(offset < _size);

	set_block(val, offset, _size - 1);
	
	if (offset > 0)
		_state = HALF_SET_CYCLE_1;

	_set_time = time;
	_set_value = val;
}


/** Set a block of buffer to @a val.
 *
 * @a start_sample and @a end_sample define the inclusive range to be set.
 */
void
AudioBuffer::set_block(Sample val, size_t start_offset, size_t end_offset)
{
	assert(end_offset >= start_offset);
	assert(end_offset < _size);
	
	Sample* const buf = data();
	assert(buf);

	for (size_t i = start_offset; i <= end_offset; ++i)
		buf[i] = val;
}


/** Scale a block of buffer by @a val.
 *
 * @a start_sample and @a end_sample define the inclusive range to be set.
 */
void
AudioBuffer::scale(Sample val, size_t start_sample, size_t end_sample)
{
	assert(end_sample >= start_sample);
	assert(end_sample < _size);
	
	Sample* const buf = data();
	assert(buf);

	for (size_t i=start_sample; i <= end_sample; ++i)
		buf[i] *= val;
}


/** Copy a block of @a src into buffer.
 *
 * @a start_sample and @a end_sample define the inclusive range to be set.
 * This function only copies the same range in one buffer to another.
 */
void
AudioBuffer::copy(const Buffer* src, size_t start_sample, size_t end_sample)
{
	assert(end_sample >= start_sample);
	assert(end_sample < _size);
	assert(src);
	assert(src->type() == DataType::CONTROL || DataType::AUDIO);
	
	Sample* const buf = data();
	assert(buf);
	
	const Sample* const src_buf = ((AudioBuffer*)src)->data();
	assert(src_buf);

	for (size_t i=start_sample; i <= end_sample; ++i)
		buf[i] = src_buf[i];
}


/** Accumulate a block of @a src into @a dst.
 *
 * @a start_sample and @a end_sample define the inclusive range to be accumulated.
 * This function only adds the same range in one buffer to another.
 */
void
AudioBuffer::accumulate(const AudioBuffer* const src, size_t start_sample, size_t end_sample)
{
	assert(end_sample >= start_sample);
	assert(end_sample < _size);
	assert(src);
	
	Sample* const buf = data();
	assert(buf);
	
	const Sample* const src_buf = src->data();
	assert(src_buf);

	for (size_t i=start_sample; i <= end_sample; ++i)
		buf[i] += src_buf[i];
	
}


/** Use another buffer's data instead of the local one.
 *
 * This buffer will essentially be identical to @a buf after this call.
 */
bool
AudioBuffer::join(Buffer* buf)
{
	AudioBuffer* abuf = dynamic_cast<AudioBuffer*>(buf);
	if (!abuf)
		return false;

	assert(abuf->size() >= _size);
	
	_joined_buf = abuf;
	_filled_size = abuf->filled_size();

	assert(_filled_size <= _size);

	return true;
}

	
void
AudioBuffer::unjoin()
{
	_joined_buf = NULL;
	_data = _local_data;
}


void
AudioBuffer::prepare_read(FrameTime start, SampleCount nframes)
{
	// FIXME: nframes parameter doesn't actually work,
	// writing starts from 0 every time
	assert(_size == 1 || nframes == _size);

	switch (_state) {
	case HALF_SET_CYCLE_1:
		if (start > _set_time)
			_state = HALF_SET_CYCLE_2;
		break;
	case HALF_SET_CYCLE_2:
		set_block(_set_value, 0, _size-1);
		_state = OK;
		break;
	default:
		break;
	}
}


/** Set the buffer (data) used.
 *
 * This is only to be used by Drivers (to provide zero-copy processing).
 */
void
AudioBuffer::set_data(Sample* buf)
{
	assert(buf);
	assert(!_joined_buf);
	_data = buf;
}


} // namespace Ingen
