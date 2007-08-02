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

#ifndef OSCBUFFER_H
#define OSCBUFFER_H

#include "Buffer.hpp"
#include "DataType.hpp"
#include "../../../../lv2/extensions/osc/lv2_osc.h"

namespace Ingen {


class OSCBuffer : public Buffer {
public:
	OSCBuffer(size_t capacity);
	
	~OSCBuffer() { }

	void clear() { lv2_osc_buffer_clear(_buf); }
	void reset(SampleCount nframes) { clear(); _this_nframes = nframes; }
	void rewind() const {}

	void prepare_read(SampleCount nframes);
	void prepare_write(SampleCount nframes);

	bool is_joined_to(Buffer* buf) const;
	bool join(Buffer* buf);
	void unjoin();
	
	void copy(const Buffer* src, size_t start_sample, size_t end_sample);

	uint32_t this_nframes() const { return _this_nframes; }

	inline LV2OSCBuffer* data()
		{ return ((_joined_buf != NULL) ? _joined_buf->data() : _buf); }
	
	inline const LV2OSCBuffer* data() const
		{ return ((_joined_buf != NULL) ? _joined_buf->data() : _buf); }

private:
	LV2OSCBuffer* const _buf;
	
	OSCBuffer* _joined_buf;  ///< Buffer to mirror, if joined

	uint32_t _this_nframes;
};


} // namespace Ingen

#endif // OSCBUFFER_H
