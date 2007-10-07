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

#include <lv2ext/lv2-midiport.h>
#include "Buffer.hpp"
#include "interface/DataType.hpp"

namespace Ingen {


class MidiBuffer : public Buffer {
public:
	MidiBuffer(size_t capacity);
	
	~MidiBuffer();

	void prepare_read(SampleCount nframes);
	void prepare_write(SampleCount nframes);

	bool is_joined_to(Buffer* buf) const;
	bool join(Buffer* buf);
	void unjoin();

	uint32_t this_nframes() const { return _this_nframes; }
	uint32_t event_count() const { return _buf->event_count; }
	
	inline LV2_MIDI* local_data() { return _local_buf; }
	
	inline void* raw_data() const
		{ return ((_joined_buf != NULL) ? _joined_buf->raw_data() : _buf); }

	inline LV2_MIDI* data()
		{ return ((_joined_buf != NULL) ? _joined_buf->data() : _buf); }
	
	inline const LV2_MIDI* data() const
		{ return ((_joined_buf != NULL) ? _joined_buf->data() : _buf); }
	
	void copy(const Buffer* src, size_t start_sample, size_t end_sample);

	inline void rewind() const { _position = 0; }
	inline void clear() { if (_joined_buf) reset(_this_nframes); }
	inline void reset(SampleCount nframes) {
		_latest_stamp = 0;
		_position = 0;
		_buf->event_count = 0;
		_buf->size = 0;
	}

	double increment() const;
	double latest_stamp() const { return _latest_stamp; }
 
	double get_event(double* timestamp, uint32_t* size, unsigned char** data) const;

	bool append(double timestamp, uint32_t size, const unsigned char* data);
	bool merge(const MidiBuffer& a, const MidiBuffer& b);

private:
	double           _latest_stamp; ///< Highest timestamp of all events
	uint32_t         _this_nframes; ///< Current cycle nframes
	mutable uint32_t _position; ///< Index into _buf
	MidiBuffer*      _joined_buf;  ///< Buffer to mirror, if joined
	LV2_MIDI*        _buf; ///< Contents (maybe belong to _joined_buf)
	LV2_MIDI*        _local_buf; ///< Local contents
};


} // namespace Ingen

#endif // MIDIBUFFER_H
