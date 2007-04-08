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

#include "MidiBuffer.h"

namespace Ingen {


/** Use another buffer's data instead of the local one.
 *
 * This buffer will essentially be identical to @a buf after this call.
 */
bool
MidiBuffer::join(Buffer* buf)
{
	MidiBuffer* mbuf = dynamic_cast<MidiBuffer*>(buf);
	if (!mbuf)
		return false;

	//assert(mbuf->size() == _size);
	
	_joined_buf = mbuf;
	
	_buf = mbuf->data();
	_state = mbuf->state();

	return true;
}

	
void
MidiBuffer::unjoin()
{
	_joined_buf = NULL;
	_buf   = _local_buf;
	_state = &_local_state;
}


bool
MidiBuffer::is_joined_to(Buffer* buf) const
{
	MidiBuffer* mbuf = dynamic_cast<MidiBuffer*>(buf);
	if (mbuf)
		return (data() == mbuf->data());

	return false;
}


void
MidiBuffer::prepare(SampleCount nframes)
{
	if (_joined_buf)
		_local_state = *_joined_buf->state();
	else
		reset_state(nframes);

	_this_nframes = nframes;
}


} // namespace Ingen
