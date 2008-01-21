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

#ifndef EVENTBUFFER_H
#define EVENTBUFFER_H

#include <iostream>
#include <lv2ext/lv2_event.h>
#include "Buffer.hpp"
#include "interface/DataType.hpp"

namespace Ingen {


class EventBuffer : public Buffer {
public:
	EventBuffer(size_t capacity);
	
	~EventBuffer();

	void prepare_read(SampleCount nframes);
	void prepare_write(SampleCount nframes);

	bool join(Buffer* buf);
	void unjoin();

	inline uint32_t this_nframes() const { return _this_nframes; }
	inline uint32_t event_count() const { return _buf->event_count; }
	
	inline void*       raw_data()       { return _buf; }
	inline const void* raw_data() const { return _buf; }

	inline LV2_Event_Buffer*       local_data() { return _local_buf; }
	inline const LV2_Event_Buffer* local_data() const { return _local_buf; }

	inline LV2_Event_Buffer*       data()       { return _buf; }
	inline const LV2_Event_Buffer* data() const { return _buf; }
	
	void copy(const Buffer* src, size_t start_sample, size_t end_sample);

	inline void rewind() const { _position = 0; }
	inline void clear() { reset(_this_nframes); }
	inline void reset(SampleCount nframes) {
		//std::cerr << this << " reset" << std::endl;
		_latest_frames = 0;
		_latest_subframes = 0;
		_position = sizeof(LV2_Event_Buffer);
		_buf->event_count = 0;
		_buf->size = 0;
	}

	bool increment() const;

	uint32_t latest_frames()    const { return _latest_frames; }
	uint32_t latest_subframes() const { return _latest_subframes; }
 
	bool get_event(uint32_t* frames,
	               uint32_t* subframes,
	               uint16_t* type,
	               uint16_t* size,
	               uint8_t** data) const;

	bool append(uint32_t       frames,
	            uint32_t       subframes,
	            uint16_t       type,
	            uint16_t       size,
	            const uint8_t* data);

	bool merge(const EventBuffer& a, const EventBuffer& b);

private:
	uint32_t          _latest_frames;    ///< Latest time of all events (frames)
	uint32_t          _latest_subframes; ///< Latest time of all events (subframes)
	uint32_t          _this_nframes;     ///< Current cycle nframes
	mutable uint32_t  _position;         ///< Offset into _buf
	LV2_Event_Buffer* _buf;              ///< Contents (maybe belong to _joined_buf)
	LV2_Event_Buffer* _local_buf;        ///< Local contents
};


} // namespace Ingen

#endif // EVENTBUFFER_H
