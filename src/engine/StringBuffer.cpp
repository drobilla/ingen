/* This file is part of Ingen.
 * Copyright (C) 2009 Dave Robillard <http://drobilla.net>
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
#include <string.h>
#include <stdint.h>
#include <iostream>
#include "ingen-config.h"
#include "StringBuffer.hpp"

using namespace std;

namespace Ingen {

using namespace Shared;

/** Allocate a new string buffer.
 * \a capacity is in bytes.
 */
StringBuffer::StringBuffer(size_t capacity)
	: Buffer(DataType(DataType::EVENT), capacity)
{
	memset(&_local_buf, '\0', sizeof(LV2_String_Data));
	_local_buf.data    = (char*)malloc(capacity);
	_local_buf.storage = capacity;

	_buf = &_local_buf;
	clear();

	cerr << "Creating String Buffer " << _buf << ", capacity = " << capacity << endl;
}


void
StringBuffer::clear()
{
	static const string default_val("2\n0 1\n1 1\n");
	if (_buf && _buf->data && _buf->storage > default_val.length())
		strncpy(_buf->data, default_val.c_str(), default_val.length());
}


/** Use another buffer's data instead of the local one.
 *
 * This buffer will essentially be identical to @a buf after this call.
 */
bool
StringBuffer::join(Buffer* buf)
{
	assert(buf != this);
	StringBuffer* sbuf = dynamic_cast<StringBuffer*>(buf);
	if (!sbuf)
		return false;

	_buf = &sbuf->_local_buf;
	_joined_buf = sbuf;

	return true;
}


void
StringBuffer::unjoin()
{
	_joined_buf = NULL;
	_buf = &_local_buf;
}


void
StringBuffer::prepare_read(FrameTime start, SampleCount nframes)
{
	_this_nframes = nframes;
}


void
StringBuffer::prepare_write(FrameTime start, SampleCount nframes)
{
}


void
StringBuffer::copy(const Buffer* src_buf, size_t start_sample, size_t end_sample)
{
	const StringBuffer* src = dynamic_cast<const StringBuffer*>(src_buf);
	assert(src);
	assert(src != this);
	assert(src->_buf != _buf);
	assert(src->_buf->data != _buf->data);

    strncpy(_buf->data, src->_buf->data, std::min(_buf->len, src->_buf->len));
	_this_nframes = end_sample - start_sample;
}


void
StringBuffer::resize(size_t size)
{
    _buf->data = (char*)realloc(_buf->data, size);
    _buf->storage = size;
	_size = size;
}


} // namespace Ingen

