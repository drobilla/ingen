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

#define __STDC_LIMIT_MACROS 1
#include <stdint.h>
#include <iostream>
#include "EventBuffer.hpp"
#include "lv2ext/lv2_event.h"

using namespace std;

namespace Ingen {


/** Allocate a new event buffer.
 * \a capacity is in bytes (not number of events).
 */
EventBuffer::EventBuffer(size_t capacity)
	: Buffer(DataType(DataType::EVENT), capacity)
	, _latest_frames(0)
	, _latest_subframes(0)
{
	if (capacity > UINT32_MAX) {
		cerr << "Event buffer size " << capacity << " too large, aborting." << endl;
		throw std::bad_alloc();
	}

	int ret = posix_memalign((void**)&_local_buf, 16, sizeof(LV2_Event_Buffer) + capacity);
	if (ret) {
		cerr << "Failed to allocate event buffer.  Aborting." << endl;
		exit(EXIT_FAILURE);
	}

	_local_buf->event_count = 0;
	_local_buf->capacity = (uint32_t)capacity;
	_local_buf->size = 0;
	_buf = _local_buf;

	reset(0);

	//cerr << "Creating MIDI Buffer " << _buf << ", capacity = " << _buf->capacity << endl;
}

EventBuffer::~EventBuffer()
{
	free(_local_buf);
}


/** Use another buffer's data instead of the local one.
 *
 * This buffer will essentially be identical to @a buf after this call.
 */
bool
EventBuffer::join(Buffer* buf)
{
	EventBuffer* mbuf = dynamic_cast<EventBuffer*>(buf);
	if (mbuf) {
		_position = mbuf->_position;
		_buf = mbuf->local_data();
		_joined_buf = mbuf;
		return false;
	} else {
		return false;
	}

	//assert(mbuf->size() == _size);
	
	_joined_buf = mbuf;

	return true;
}

	
void
EventBuffer::unjoin()
{
	_joined_buf = NULL;
	_buf = _local_buf;
	reset(_this_nframes);
}


void
EventBuffer::prepare_read(SampleCount nframes)
{
	//cerr << "\t" << this << " prepare_read: " << event_count() << endl;

	rewind();
	_this_nframes = nframes;
}


void
EventBuffer::prepare_write(SampleCount nframes)
{
	//cerr << "\t" << this << " prepare_write: " << event_count() << endl;
	reset(nframes);
}
	
/** FIXME: parameters ignored */
void
EventBuffer::copy(const Buffer* src_buf, size_t start_sample, size_t end_sample)
{
	const EventBuffer* src = dynamic_cast<const EventBuffer*>(src_buf);
	assert(src);
	assert(_buf->capacity >= src->_buf->capacity);
	
	clear();
	src->rewind();

	memcpy(_buf, src->_buf, src->_buf->size);
}


/** Increment the read position by one event.
 *
 * \return true if increment was successful, or false if end of buffer reached.
 */
bool
EventBuffer::increment() const
{
	if (_position + sizeof(LV2_Event) >= _buf->size) {
		_position = _buf->size;
		return false;
	}

	const LV2_Event* ev = (const LV2_Event*)(_buf + sizeof(LV2_Event_Buffer) + _position);

	_position += sizeof(LV2_Event) + ev->size;

	if (_position >= _buf->size) {
		_position = _buf->size;
		return false;
	} else {
		ev = (const LV2_Event*)(_buf + sizeof(LV2_Event_Buffer) + _position);
		return true;
	}
}


/** Append an event to the buffer.
 *
 * \a timestamp must be >= the latest event in the buffer,
 * and < this_nframes()
 *
 * \return true on success
 */
bool
EventBuffer::append(uint32_t       frames,
                    uint32_t       subframes,
                    uint16_t       type,
                    uint16_t       size,
                    const uint8_t* data)
{
	/*cerr << "Append event " << size << " bytes @ " << timestamp << ":" << endl;
	for (uint32_t i=0; i < size; ++i) {
		fprintf(stderr, "( %X )", *((uint8_t*)data + i));
	}*/

	if (_buf->capacity - _buf->size < sizeof(LV2_Event) + size)
		return false;
	
	assert(size > 0);
	assert(frames > _latest_frames
			|| (frames == _latest_frames && subframes >= _latest_subframes));

	LV2_Event* ev = (LV2_Event*)(_buf + sizeof(LV2_Event_Buffer) + _position);
	_position += sizeof(LV2_Event) + ev->size;

	ev = (LV2_Event*)(_buf + sizeof(LV2_Event_Buffer) + _position);
	
	ev->frames = frames;
	ev->subframes = subframes;
	ev->type = type;
	ev->size = size;
	memcpy((uint8_t*)ev + sizeof(LV2_Event), data, size);
	
	_buf->size += sizeof(LV2_Event) + size;
	++_buf->event_count;
	
	_latest_frames = frames;
	_latest_subframes = subframes;

	return true;
}


/** Read an event from the current position in the buffer
 * 
 * \return true if read was successful, or false if end of buffer reached
 */
bool
EventBuffer::get_event(uint32_t* frames,
                       uint32_t* subframes, 
                       uint16_t* type, 
                       uint16_t* size, 
                       uint8_t** data) const
{
	const LV2_Event_Buffer* const buf = this->data();

	if (_position >= buf->size) {
		_position = buf->size;
		*size = 0;
		*data = NULL;
		return false;
	}
	
	const LV2_Event* const ev = (const LV2_Event*)
		(_buf + sizeof(LV2_Event_Buffer) + _position);

	*frames = ev->frames;
	*subframes = ev->subframes;
	*type = ev->type;
	*size = ev->size;
	*data = (uint8_t*)ev + sizeof(LV2_Event);

	return true;
}


/** Clear, and merge \a a and \a b into this buffer.
 *
 * FIXME: This is slow.
 *
 * \return true if complete merge was successful
 */
bool
EventBuffer::merge(const EventBuffer& a, const EventBuffer& b)
{
	// Die if a merge isn't necessary as it's expensive
	assert(a.size() > 0 && b.size() > 0);

	reset(_this_nframes);

	a.rewind();
	b.rewind();

#if 0
	uint32_t a_frames;
	uint32_t a_subframes;
	uint16_t a_type;
	uint16_t a_size;
	uint8_t* a_data;
	
	uint32_t b_frames;
	uint32_t b_subframes;
	uint16_t b_type;
	uint16_t b_size;
	uint8_t* b_data;
#endif
	
	cout << "FIXME: merge" << endl;
#if 0
	a.get_event(&a_frames, &a_subframes, &a_type, &a_size, &a_data);
	b.get_event(&b_frames, &b_subframes, &b_type, &b_size, &b_data);

	while (true) {
		if (a_data && (!b_data || (a_time < b_time))) {
			append(a_time, a_size, a_data);
			if (a.increment())
				a.get_event(&a_time, &a_size, &a_data);
			else
				a_data = NULL;
		} else if (b_data) {
			append(b_time, b_size, b_data);
			if (b.increment())
				b.get_event(&b_time, &b_size, &b_data);
			else
				b_data = NULL;
		} else {
			break;
		}
	}

	_latest_stamp = max(a_time, b_time);
#endif

	return true;
}


} // namespace Ingen

