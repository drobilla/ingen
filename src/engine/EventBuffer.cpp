/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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
#include "ingen-config.h"
#include "EventBuffer.hpp"
#include "lv2ext/lv2_event.h"
#include "lv2ext/lv2_event_helpers.h"

using namespace std;

namespace Ingen {

using namespace Shared;

/** Allocate a new event buffer.
 * \a capacity is in bytes (not number of events).
 */
EventBuffer::EventBuffer(size_t capacity)
	: Buffer(DataType(DataType::EVENT), capacity)
	, _local_buf(new LV2EventBuffer(capacity))
{
	_buf = _local_buf;
	reset(0);

	//cerr << "Creating MIDI Buffer " << _buf << ", capacity = " << _buf->capacity << endl;
}


/** Use another buffer's data instead of the local one.
 *
 * This buffer will essentially be identical to @a buf after this call.
 */
bool
EventBuffer::join(Buffer* buf)
{
	assert(buf != this);
	EventBuffer* ebuf = dynamic_cast<EventBuffer*>(buf);
	if (!ebuf)
		return false;

	_buf = ebuf->_local_buf;
	_joined_buf = ebuf;

	return true;
}


void
EventBuffer::unjoin()
{
	_joined_buf = NULL;
	_buf = _local_buf;
}


void
EventBuffer::prepare_read(FrameTime start, SampleCount nframes)
{
	rewind();
	_this_nframes = nframes;
}


void
EventBuffer::prepare_write(FrameTime start, SampleCount nframes)
{
	reset(nframes);
}

/** FIXME: parameters ignored */
void
EventBuffer::copy(const Buffer* src_buf, size_t start_sample, size_t end_sample)
{
	const EventBuffer* src = dynamic_cast<const EventBuffer*>(src_buf);
	assert(src);
	assert(_buf->capacity() >= src->_buf->capacity());
	assert(src != this);
	assert(src->_buf != _buf);

	src->rewind();

	_buf->copy(*src->_buf);
	_this_nframes = end_sample - start_sample;
	assert(event_count() == src->event_count());
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

