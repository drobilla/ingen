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

#ifndef EVENTBUFFER_H
#define EVENTBUFFER_H

#include "event.lv2/event.h"
#include "event.lv2/event-helpers.h"
#include "interface/DataType.hpp"
#include "Buffer.hpp"
#include "LV2EventBuffer.hpp"

namespace Ingen {


class EventBuffer : public Buffer {
public:
	EventBuffer(size_t capacity);

	void clear() { _buf->reset(); }

	void*       port_data(Shared::DataType port_type)       { return _buf; }
	const void* port_data(Shared::DataType port_type) const { return _buf; }

	void rewind() const { _buf->rewind(); }

	void prepare_read(Context& context);
	void prepare_write(Context& context);

	void copy(Context& context, const Buffer* src);
	void mix(Context& contect, const Buffer* src);
	bool merge(const EventBuffer& a, const EventBuffer& b);

	bool increment() const { return _buf->increment(); }
	bool is_valid()  const { return _buf->is_valid(); }

	inline uint32_t latest_frames()    const { return _buf->latest_frames(); }
	inline uint32_t latest_subframes() const { return _buf->latest_subframes(); }
	inline uint32_t event_count()      const { return _buf->event_count(); }

	inline bool get_event(uint32_t* frames,
	                      uint32_t* subframes,
	                      uint16_t* type,
	                      uint16_t* size,
	                      uint8_t** data) const {
		return _buf->get_event(frames, subframes, type, size, data);
	}

	inline bool append(uint32_t       frames,
	                   uint32_t       subframes,
	                   uint16_t       type,
	                   uint16_t       size,
	                   const uint8_t* data) {
		return _buf->append(frames, subframes, type, size, data);
	}

	inline bool append(const LV2_Event_Buffer* buf) {
		return _buf->append(buf);
	}

private:
	LV2EventBuffer* _buf; ///< Contents
};


} // namespace Ingen

#endif // EVENTBUFFER_H
