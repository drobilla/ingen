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

#include <iostream>
#include "OSCBuffer.hpp"

using namespace std;

namespace Ingen {

OSCBuffer::OSCBuffer(size_t capacity)
	: Buffer(DataType(DataType::OSC), capacity)
	, _buf(lv2_osc_buffer_new((uint32_t)capacity))
	, _joined_buf(NULL)
{
	/*_local_state.midi = _buf;
	_state = &_local_state;
	assert(_local_state.midi);
	reset(0);
	clear();
	assert(_local_state.midi == _buf);
*/
	//cerr << "Creating OSC Buffer " << _buf << ", capacity = " << _buf->capacity << endl;
}



/** Use another buffer's data instead of the local one.
 *
 * This buffer will essentially be identical to @a buf after this call.
 */
bool
OSCBuffer::join(Buffer* buf)
{
	OSCBuffer* obuf = dynamic_cast<OSCBuffer*>(buf);
	if (!obuf)
		return false;

	//assert(mbuf->size() == _size);
	
	_joined_buf = obuf;
	
	//cerr << "OSC buffer joined" << endl;

	//_state = mbuf->_state;

	return true;
}

	
void
OSCBuffer::unjoin()
{
	_joined_buf = NULL;
	//_state = &_local_state;
	//_state->midi = _buf;

	reset(_this_nframes);
}


bool
OSCBuffer::is_joined_to(Buffer* buf) const
{
	OSCBuffer* obuf = dynamic_cast<OSCBuffer*>(buf);
	if (obuf)
		return (data() == obuf->data());

	return false;
}


void
OSCBuffer::prepare_read(SampleCount nframes)
{
	assert(!_joined_buf || data() == _joined_buf->data());
	
	reset(nframes);
}


void
OSCBuffer::prepare_write(SampleCount nframes)
{
	clear();
	reset(nframes);

	assert(!_joined_buf || data() == _joined_buf->data());
}


void
OSCBuffer::copy(const Buffer* src, size_t start_sample, size_t end_sample)
{
	// FIXME
	//cerr << "WARNING: OSC buffer copying not implemented, data dropped." << endl;
}



} // namespace Ingen
