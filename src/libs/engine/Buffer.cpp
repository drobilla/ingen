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

#include "Buffer.h"
#include <iostream>
#include <cassert>
#include <stdlib.h>
#include "MidiMessage.h"
using std::cerr; using std::endl;

/* TODO: Be sure these functions are vectorized by GCC when it's vectorizer
 * stops sucking.  Probably a good idea to inline them as well */

namespace Ingen {


template <typename T>
Buffer<T>::Buffer(size_t size)
: _data(NULL),
  _local_data(NULL),
  _joined_buf(NULL),
  _size(size),
  _filled_size(0),
  _state(OK),
  _set_value(0)
{
	assert(_size > 0);
	allocate();
	assert(data());
}
template Buffer<Sample>::Buffer(size_t size);
template Buffer<MidiMessage>::Buffer(size_t size);


template<typename T>
void
Buffer<T>::resize(size_t size)
{
	_size = size;

	T* const old_data = _data;
	
	const bool using_local_data = (_data == _local_data);

	deallocate();

	const int ret = posix_memalign((void**)&_local_data, 16, _size * sizeof(T));
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

	set(0, 0, _size-1);
}
template void Buffer<Sample>::resize(size_t size);
template void Buffer<MidiMessage>::resize(size_t size);


/** Allocate and use a locally managed buffer (data).
 */
template<typename T>
void
Buffer<T>::allocate()
{
	assert(!_joined_buf);
	assert(_local_data == NULL);
	assert(_size > 0);

	const int ret = posix_memalign((void**)&_local_data, 16, _size * sizeof(T));
	if (ret != 0) {
		cerr << "[Buffer] Failed to allocate buffer.  Aborting." << endl;
		exit(EXIT_FAILURE);
	}
		
	assert(ret == 0);
	assert(_local_data);

	_data = _local_data;

	set(0, 0, _size-1);
}
template void Buffer<Sample>::allocate();
template void Buffer<MidiMessage>::allocate();


/** Free locally allocated buffer.
 */
template<typename T>
void
Buffer<T>::deallocate()
{
	assert(!_joined_buf);
	free(_local_data);
	_local_data = NULL;
	_data = NULL;
}
template void Buffer<Sample>::deallocate();
template void Buffer<MidiMessage>::deallocate();


/** Empty (ie zero) the buffer.
 */
template<typename T>
void
Buffer<T>::clear()
{
	set(0, 0, _size-1);
	_state = OK;
	_filled_size = 0;
}
template void Buffer<Sample>::clear();
template void Buffer<MidiMessage>::clear();


/** Set value of buffer to @a val after @a start_sample.
 *
 * The Buffer will handle setting the intial portion of the buffer to the
 * value on the next cycle automatically (if @a start_sample is > 0), as
 * long as pre_process() is called every cycle.
 */
template <typename T>
void
Buffer<T>::set(T val, size_t start_sample)
{
	assert(start_sample < _size);

	set(val, start_sample, _size-1);
	
	if (start_sample > 0)
		_state = HALF_SET_CYCLE_1;

	_set_value = val;
}
template void Buffer<Sample>::set(Sample val, size_t start_sample);
template void Buffer<MidiMessage>::set(MidiMessage val, size_t start_sample);


/** Set a block of buffer to @a val.
 *
 * @a start_sample and @a end_sample define the inclusive range to be set.
 */
template <typename T>
void
Buffer<T>::set(T val, size_t start_sample, size_t end_sample)
{
	assert(end_sample >= start_sample);
	assert(end_sample < _size);
	
	T* const buf = data();
	assert(buf);

	for (size_t i=start_sample; i <= end_sample; ++i)
		buf[i] = val;
}
template void Buffer<Sample>::set(Sample val, size_t start_sample, size_t end_sample);
template void Buffer<MidiMessage>::set(MidiMessage val, size_t start_sample, size_t end_sample);


/** Scale a block of buffer by @a val.
 *
 * @a start_sample and @a end_sample define the inclusive range to be set.
 */
template <typename T>
void
Buffer<T>::scale(T val, size_t start_sample, size_t end_sample)
{
	assert(end_sample >= start_sample);
	assert(end_sample < _size);
	
	T* const buf = data();
	assert(buf);

	for (size_t i=start_sample; i <= end_sample; ++i)
		buf[i] *= val;
}
template void Buffer<Sample>::scale(Sample val, size_t start_sample, size_t end_sample);


/** Copy a block of @a src into buffer.
 *
 * @a start_sample and @a end_sample define the inclusive range to be set.
 * This function only copies the same range in one buffer to another.
 */
template <typename T>
void
Buffer<T>::copy(const Buffer<T>* src, size_t start_sample, size_t end_sample)
{
	assert(end_sample >= start_sample);
	assert(end_sample < _size);
	assert(src);
	
	T* const buf = data();
	assert(buf);
	
	const T* const src_buf = src->data();
	assert(src_buf);

	for (size_t i=start_sample; i <= end_sample; ++i)
		buf[i] = src_buf[i];
}
template void Buffer<Sample>::copy(const Buffer<Sample>* const src, size_t start_sample, size_t end_sample);
template void Buffer<MidiMessage>::copy(const Buffer<MidiMessage>* const src, size_t start_sample, size_t end_sample);


/** Accumulate a block of @a src into @a dst.
 *
 * @a start_sample and @a end_sample define the inclusive range to be accumulated.
 * This function only adds the same range in one buffer to another.
 */
template <typename T>
void
Buffer<T>::accumulate(const Buffer<T>* const src, size_t start_sample, size_t end_sample)
{
	assert(end_sample >= start_sample);
	assert(end_sample < _size);
	assert(src);
	
	T* const buf = data();
	assert(buf);
	
	const T* const src_buf = src->data();
	assert(src_buf);

	for (size_t i=start_sample; i <= end_sample; ++i)
		buf[i] += src_buf[i];
	
}
template void Buffer<Sample>::accumulate(const Buffer<Sample>* const src, size_t start_sample, size_t end_sample);


/** Use another buffer's data instead of the local one.
 *
 * This buffer will essentially be identical to @a buf after this call.
 */
template<typename T>
void
Buffer<T>::join(Buffer<T>* buf)
{
	assert(buf->size() == _size);
	
	_joined_buf = buf;
	_filled_size = buf->filled_size();

	assert(_filled_size <= _size);
}
template void Buffer<Sample>::join(Buffer<Sample>* buf);
template void Buffer<MidiMessage>::join(Buffer<MidiMessage>* buf);

	
template<typename T>
void
Buffer<T>::unjoin()
{
	_joined_buf = NULL;
	_data = _local_data;
}
template void Buffer<Sample>::unjoin();
template void Buffer<MidiMessage>::unjoin();


template<>
void
Buffer<Sample>::prepare(SampleCount nframes)
{
	// FIXME: nframes parameter doesn't actually work,
	// writing starts from 0 every time
	assert(_size == 1 || nframes == _size);

	switch (_state) {
	case HALF_SET_CYCLE_1:
		_state = HALF_SET_CYCLE_2;
		break;
	case HALF_SET_CYCLE_2:
		set(_set_value, 0, _size-1);
		_state = OK;
		break;
	default:
		break;
	}
}


template<>
void
Buffer<MidiMessage>::prepare(SampleCount nframes)
{
}


/** Set the buffer (data) used.
 *
 * This is only to be used by Drivers (to provide zero-copy processing).
 */
template<typename T>
void
Buffer<T>::set_data(T* buf)
{
	assert(buf);
	assert(!_joined_buf);
	_data = buf;
}
template void Buffer<Sample>::set_data(Sample* data);
template void Buffer<MidiMessage>::set_data(MidiMessage* data);


} // namespace Ingen
