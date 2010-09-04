/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#ifndef INGEN_ENGINE_EVENTBUFFER_HPP
#define INGEN_ENGINE_EVENTBUFFER_HPP

#include "event.lv2/event.h"
#include "event.lv2/event-helpers.h"
#include "atom.lv2/atom.h"
#include "interface/PortType.hpp"
#include "Buffer.hpp"

namespace Ingen {


class EventBuffer : public Buffer {
public:
	EventBuffer(BufferFactory& bufs, size_t capacity);
	~EventBuffer();

	void*       port_data(Shared::PortType port_type, SampleCount offset=0)       { return _data; }
	const void* port_data(Shared::PortType port_type, SampleCount offset=0) const { return _data; }

	inline void rewind() const { lv2_event_begin(&_iter, _data); }

	inline void clear() {
		_latest_frames = 0;
		_latest_subframes = 0;
		_data->event_count = 0;
		_data->size = 0;
		rewind();
	}

	void prepare_read(Context& context);
	void prepare_write(Context& context);

	void copy(Context& context, const Buffer* src);

	inline size_t   event_count()      const { return _data->event_count; }
	inline uint32_t capacity()         const { return _data->capacity; }
	inline uint32_t latest_frames()    const { return _latest_frames; }
	inline uint32_t latest_subframes() const { return _latest_subframes; }

	bool increment() const;
	bool is_valid()  const;

	bool get_event(uint32_t* frames,
	               uint32_t* subframes,
	               uint16_t* type,
	               uint16_t* size,
	               uint8_t** data) const;

	LV2_Atom*   get_atom() const;
	LV2_Event*  get_event() const;

	bool append(uint32_t       frames,
	            uint32_t       subframes,
	            uint16_t       type,
	            uint16_t       size,
	            const uint8_t* data);

private:
	LV2_Event_Buffer*          _data;             ///< Contents
	mutable LV2_Event_Iterator _iter;             ///< Iterator into _data
	uint32_t                   _latest_frames;    ///< Latest time of all events (frames)
	uint32_t                   _latest_subframes; ///< Latest time of all events (subframes)
};


} // namespace Ingen

#endif // INGEN_ENGINE_EVENTBUFFER_HPP
