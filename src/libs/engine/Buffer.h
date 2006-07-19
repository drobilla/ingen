/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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

#ifndef BUFFER_H
#define BUFFER_H

#include <cstddef>
#include <cassert>
#include "types.h"

namespace Ingen {

	
template <typename T>
class Buffer
{
public:
	Buffer(size_t size);

	void clear();
	void set(T val, size_t start_sample);
	void set(T val, size_t start_sample, size_t end_sample);
	void scale(T val, size_t start_sample, size_t end_sample);
	void copy(const Buffer<T>* src, size_t start_sample, size_t end_sample);
	void accumulate(const Buffer<T>* src, size_t start_sample, size_t end_sample);
	
	void join(Buffer* buf);
	void unjoin();
	
	/** For driver use only!! */
	void set_data(T* data);
	
	inline T& value_at(size_t offset) { assert(offset < m_size); return m_data[offset]; }
	
	void prepare(SampleCount nframes);
	
	void     filled_size(size_t size) { m_filled_size = size; }
	size_t   filled_size() const { return m_filled_size; }
	bool     is_joined()   const { return m_is_joined; }
	size_t   size()        const { return m_size; }
	T*       data()        const { return m_data; }

protected:
	enum BufferState { OK, HALF_SET_CYCLE_1, HALF_SET_CYCLE_2 };

	void allocate();
	void deallocate();

	size_t      m_size;          ///< Allocated buffer size
	size_t      m_filled_size;   ///< Usable buffer size (for MIDI ports etc)
	bool        m_is_joined;     ///< Whether or not @ref m_data is shares with another Buffer
	BufferState m_state;         ///< State of buffer for setting values next cycle
	T           m_set_value;     ///< Value set by @ref set (may need to be set next cycle)

	T* m_data; ///< Buffer to be returned by data() (not equal to m_local_data if joined)
	T* m_local_data; ///< Locally allocated buffer
};


/** Less robust Buffer for Driver's use.
 *
 * Does not allocate an array by default, and allows direct setting of
 * data pointer for zero-copy processing.
 */
template <typename T>
class DriverBuffer : public Buffer<T>
{
public:
	DriverBuffer(size_t size);
	

	
private:
	using Buffer<T>::m_data;
	using Buffer<T>::m_is_joined;
};


} // namespace Ingen

#endif // BUFFER_H
