/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
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

namespace Om {


template <typename T>
Buffer<T>::Buffer(size_t size)
: m_size(size),
  m_filled_size(0),
  m_is_joined(false),
  m_state(OK),
  m_set_value(0),
  m_data(NULL),
  m_local_data(NULL)
{
	assert(m_size > 0);
	allocate();
	assert(m_data);
}
template Buffer<sample>::Buffer(size_t size);
template Buffer<MidiMessage>::Buffer(size_t size);


/** Allocate and use a locally managed buffer (data).
 */
template<typename T>
void
Buffer<T>::allocate()
{
	assert(!m_is_joined);
	assert(m_data == NULL);
	assert(m_local_data == NULL);
	assert(m_size > 0);

	const int ret = posix_memalign((void**)&m_local_data, 16, m_size * sizeof(T));
	if (ret != 0) {
		cerr << "[Buffer] Failed to allocate buffer.  Aborting." << endl;
		exit(EXIT_FAILURE);
	}
		
	assert(ret == 0);
	assert(m_local_data != NULL);
	m_data = m_local_data;

	set(0, 0, m_size-1);
}
template void Buffer<sample>::allocate();
template void Buffer<MidiMessage>::allocate();


/** Free locally allocated buffer.
 */
template<typename T>
void
Buffer<T>::deallocate()
{
	assert(!m_is_joined);
	free(m_local_data);
	if (m_data == m_local_data)
		m_data = NULL;
	m_local_data = NULL;
}
template void Buffer<sample>::deallocate();
template void Buffer<MidiMessage>::deallocate();


/** Empty (ie zero) the buffer.
 */
template<typename T>
void
Buffer<T>::clear()
{
	set(0, 0, m_size-1);
	m_state = OK;
	m_filled_size = 0;
}
template void Buffer<sample>::clear();
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
	assert(start_sample < m_size);

	set(val, start_sample, m_size-1);
	
	if (start_sample > 0)
		m_state = HALF_SET_CYCLE_1;

	m_set_value = val;
}
template void Buffer<sample>::set(sample val, size_t start_sample);
template void Buffer<MidiMessage>::set(MidiMessage val, size_t start_sample);


/** Set a block of buffer to @a val.
 *
 * @a start_sample and @a end_sample define the inclusive range to be set.
 */
template <typename T>
void
Buffer<T>::set(T val, size_t start_sample, size_t end_sample)
{
	assert(start_sample >= 0);
	assert(end_sample >= start_sample);
	assert(end_sample < m_size);
	assert(m_data != NULL);

	for (size_t i=start_sample; i <= end_sample; ++i)
		m_data[i] = val;
}
template void Buffer<sample>::set(sample val, size_t start_sample, size_t end_sample);
template void Buffer<MidiMessage>::set(MidiMessage val, size_t start_sample, size_t end_sample);


/** Scale a block of buffer by @a val.
 *
 * @a start_sample and @a end_sample define the inclusive range to be set.
 */
template <typename T>
void
Buffer<T>::scale(T val, size_t start_sample, size_t end_sample)
{
	assert(start_sample >= 0);
	assert(end_sample >= start_sample);
	assert(end_sample < m_size);
	assert(m_data != NULL);

	for (size_t i=start_sample; i <= end_sample; ++i)
		m_data[i] *= val;
}
template void Buffer<sample>::scale(sample val, size_t start_sample, size_t end_sample);


/** Copy a block of @a src into buffer.
 *
 * @a start_sample and @a end_sample define the inclusive range to be set.
 * This function only copies the same range in one buffer to another.
 */
template <typename T>
void
Buffer<T>::copy(const Buffer<T>* src, size_t start_sample, size_t end_sample)
{
	assert(start_sample >= 0);
	assert(end_sample >= start_sample);
	assert(end_sample < m_size);
	assert(src != NULL);
	assert(src->data() != NULL);
	assert(m_data != NULL);
	
	register const T* const src_data = src->data();

	for (size_t i=start_sample; i <= end_sample; ++i)
		m_data[i] = src_data[i];
}
template void Buffer<sample>::copy(const Buffer<sample>* const src, size_t start_sample, size_t end_sample);
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
	assert(start_sample >= 0);
	assert(end_sample >= start_sample);
	assert(end_sample < m_size);
	assert(src != NULL);
	assert(src->data() != NULL);
	assert(m_data != NULL);

	register const T* const src_data = src->data();
	
	for (size_t i=start_sample; i <= end_sample; ++i)
		m_data[i] += src_data[i];
	
}
template void Buffer<sample>::accumulate(const Buffer<sample>* const src, size_t start_sample, size_t end_sample);


/** Use another buffer's data instead of the local one.
 *
 * This buffer will essentially be identical to @a buf after this call.
 */
template<typename T>
void
Buffer<T>::join(Buffer<T>* buf)
{
	assert(buf->size() == m_size);
	
	m_data = buf->m_data;
	m_filled_size = buf->filled_size();
	m_is_joined = true;

	assert(m_filled_size <= m_size);
}
template void Buffer<sample>::join(Buffer<sample>* buf);
template void Buffer<MidiMessage>::join(Buffer<MidiMessage>* buf);

	
template<typename T>
void
Buffer<T>::unjoin()
{
	m_is_joined = false;
	m_data = m_local_data;
}
template void Buffer<sample>::unjoin();
template void Buffer<MidiMessage>::unjoin();


template<>
void
Buffer<sample>::prepare(samplecount nframes)
{
	// FIXME: nframes parameter doesn't actually work,
	// writing starts from 0 every time
	assert(m_size == 1 || nframes == m_size);

	switch (m_state) {
	case HALF_SET_CYCLE_1:
		m_state = HALF_SET_CYCLE_2;
		break;
	case HALF_SET_CYCLE_2:
		set(m_set_value, 0, m_size-1);
		m_state = OK;
		break;
	default:
		break;
	}
}


template<>
void
Buffer<MidiMessage>::prepare(samplecount nframes)
{
}


/** Set the buffer (data) used.
 *
 * This is only to be used by Drivers (to provide zero-copy processing).
 */
template<typename T>
void
Buffer<T>::set_data(T* data)
{
	assert(!m_is_joined);
	m_data = data;
}
template void Buffer<sample>::set_data(sample* data);
template void Buffer<MidiMessage>::set_data(MidiMessage* data);


////// DriverBuffer ////////
#if 0
template <typename T>
DriverBuffer<T>::DriverBuffer(size_t size)
: Buffer<T>(size)
{
	Buffer<T>::deallocate(); // FIXME: allocate then immediately deallocate, dirty
	Buffer<T>::m_data = NULL;
}
template DriverBuffer<sample>::DriverBuffer(size_t size);
template DriverBuffer<MidiMessage>::DriverBuffer(size_t size);


/** Set the buffer (data) used.
 *
 * This is only to be used by Drivers (to provide zero-copy processing).
 */
template<typename T>
void
DriverBuffer<T>::set_data(T* data)
{
	assert(!m_is_joined);
	m_data = data;
}
template void DriverBuffer<sample>::set_data(sample* data);
template void DriverBuffer<MidiMessage>::set_data(MidiMessage* data);
#endif

} // namespace Om
