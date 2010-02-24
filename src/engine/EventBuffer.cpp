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
#include "raul/log.hpp"
#include "event.lv2/event.h"
#include "event.lv2/event-helpers.h"
#include "ingen-config.h"
#include "EventBuffer.hpp"
#include "ProcessContext.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {

using namespace Shared;

/** Allocate a new event buffer.
 * \a capacity is in bytes (not number of events).
 */
EventBuffer::EventBuffer(BufferFactory& bufs, size_t capacity)
	: Buffer(bufs, PortType(PortType::EVENTS), capacity)
	, _latest_frames(0)
	, _latest_subframes(0)
{
	if (capacity > UINT32_MAX) {
		error << "Event buffer size " << capacity << " too large, aborting." << endl;
		throw std::bad_alloc();
	}

#ifdef HAVE_POSIX_MEMALIGN
	int ret = posix_memalign((void**)&_data, 16, sizeof(LV2_Event_Buffer) + capacity);
#else
	_data = (LV2_Event_Buffer*)malloc(sizeof(LV2_Event_Buffer) + capacity);
	int ret = (_data != NULL) ? 0 : -1;
#endif

	if (ret != 0) {
		error << "Failed to allocate event buffer.  Aborting." << endl;
		exit(EXIT_FAILURE);
	}

	_data->header_size = sizeof(LV2_Event_Buffer);
	_data->data = reinterpret_cast<uint8_t*>(_data + _data->header_size);
	_data->stamp_type = 0;
	_data->event_count = 0;
	_data->capacity = (uint32_t)capacity;
	_data->size = 0;

	clear();
}


EventBuffer::~EventBuffer()
{
	free(_data);
}


void
EventBuffer::prepare_read(Context& context)
{
	rewind();
}


void
EventBuffer::prepare_write(Context& context)
{
	if (context.offset() == 0)
		clear();
}


void
EventBuffer::copy(Context& context, const Buffer* src_buf)
{
	const EventBuffer* src = dynamic_cast<const EventBuffer*>(src_buf);
	if (src->_data == _data)
		return;

	assert(src->_data->header_size == _data->header_size);
	assert(capacity() >= _data->header_size + src->_data->size);

	rewind();

	memcpy(_data, src->_data, _data->header_size + src->_data->size);

	_iter = src->_iter;
	_iter.buf = _data;

	_latest_frames    = src->_latest_frames;
	_latest_subframes = src->_latest_subframes;

	assert(event_count() == src->event_count());
}


/** Increment the read position by one event.
 *
 * \return true if increment was successful, or false if end of buffer reached.
 */
bool
EventBuffer::increment() const
{
	if (lv2_event_is_valid(&_iter)) {
		lv2_event_increment(&_iter);
		return true;
	} else {
		return false;
	}
}


/** \return true iff the cursor is valid (ie get_event is safe)
 */
bool
EventBuffer::is_valid() const
{
	return lv2_event_is_valid(&_iter);
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
	if (lv2_event_is_valid(&_iter)) {
		LV2_Event* ev = lv2_event_get(&_iter, data);
		*frames = ev->frames;
		*subframes = ev->subframes;
		*type = ev->type;
		*size = ev->size;
		return true;
	} else {
		return false;
	}
}


/** Get the object currently pointed to, or NULL if invalid.
 */
LV2_Object*
EventBuffer::get_object() const
{
	if (lv2_event_is_valid(&_iter)) {
		uint8_t* data;
		LV2_Event* ev = lv2_event_get(&_iter, &data);
		return LV2_OBJECT_FROM_EVENT(ev);
	}
	return NULL;
}


/** Get the event currently pointed to, or NULL if invalid.
 */
LV2_Event*
EventBuffer::get_event() const
{
	if (lv2_event_is_valid(&_iter)) {
		uint8_t* data;
		return lv2_event_get(&_iter, &data);
	}
	return NULL;
}


/** Append an event to the buffer.
 *
 * \a timestamp must be >= the latest event in the buffer.
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
#ifndef NDEBUG
	if (lv2_event_is_valid(&_iter)) {
		LV2_Event* last_event = lv2_event_get(&_iter, NULL);
		assert(last_event->frames < frames
				|| (last_event->frames == frames && last_event->subframes <= subframes));
	}
#endif

	/*debug << "Appending event type " << type << ", size " << size
		<< " @ " << frames << "." << subframes << endl;*/

	if (!lv2_event_write(&_iter, frames, subframes, type, size, data)) {
		error << "Failed to write event." << endl;
		return false;
	} else {
		_latest_frames = frames;
		_latest_subframes = subframes;
		return true;
	}
}


} // namespace Ingen
