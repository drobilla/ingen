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

#ifndef MIDIBUFFER_H
#define MIDIBUFFER_H

#include <lv2ext/lv2-midifunctions.h>
#include "Buffer.hpp"
#include "DataType.hpp"

namespace Ingen {


class MidiBuffer : public Buffer {
public:
	MidiBuffer(size_t capacity);
	
	~MidiBuffer() { lv2midi_free(_local_state.midi); }

	void prepare_read(SampleCount nframes);
	void prepare_write(SampleCount nframes);

	bool is_joined_to(Buffer* buf) const;
	bool join(Buffer* buf);
	void unjoin();

	uint32_t this_nframes() const { return _this_nframes; }
	
	inline LV2_MIDI* data()
		{ return ((_joined_buf != NULL) ? _joined_buf->data() : _state->midi); }
	
	inline const LV2_MIDI* data() const
		{ return ((_joined_buf != NULL) ? _joined_buf->data() : _state->midi); }

	inline void clear()
		{ assert(_state); assert(_state->midi); lv2midi_reset_buffer(_state->midi); _state->position = 0; }

	inline void reset(SampleCount nframes)
		{ assert(_state); assert(_state->midi); lv2midi_reset_state(_state, _state->midi, nframes); _this_nframes = nframes; }

	inline double increment()
		{ assert(_state); assert(_state->midi); return lv2midi_step(_state); }
 
	inline double get_event(double* timestamp, uint32_t* size, unsigned char** data)
		{ assert(_state); assert(_state->midi); return lv2midi_get_event(_state, timestamp, size, data); }

	inline int put_event(double timestamp, uint32_t size, const unsigned char* data)
		{ assert(_state); assert(_state->midi); return lv2midi_put_event(_state, timestamp, size, data); }

private:
	LV2_MIDIState    _local_state;
	LV2_MIDIState*   _state;
	LV2_MIDI* const  _buf;
	
	MidiBuffer* _joined_buf;  ///< Buffer to mirror, if joined

	uint32_t _this_nframes;
};


} // namespace Ingen

#endif // MIDIBUFFER_H
