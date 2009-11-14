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
#include <algorithm>
#include <iostream>
#include "uri-map.lv2/uri-map.h"
#include "ingen-config.h"
#include "shared/LV2Features.hpp"
#include "shared/LV2URIMap.hpp"
#include "ObjectBuffer.hpp"
#include "Engine.hpp"

using namespace std;

namespace Ingen {

using namespace Shared;


/** Allocate a new string buffer.
 * \a capacity is in bytes.
 */
ObjectBuffer::ObjectBuffer(size_t capacity)
	: Buffer(DataType(DataType::OBJECT), capacity)
{
	capacity = std::max(capacity, (size_t)32);
	cerr << "Creating Object Buffer " << _buf << " capacity = " << capacity << endl;
	_local_buf = (LV2_Object*)malloc(sizeof(LV2_Object) + capacity);
	_buf = _local_buf;
	clear();
}


void
ObjectBuffer::clear()
{
	// nil
	_buf->type = 0;
	_buf->size = 0;
}


/** Use another buffer's data instead of the local one.
 *
 * This buffer will essentially be identical to @a buf after this call.
 */
bool
ObjectBuffer::join(Buffer* buf)
{
	assert(buf != this);
	ObjectBuffer* sbuf = dynamic_cast<ObjectBuffer*>(buf);
	if (!sbuf)
		return false;

	_buf = sbuf->_local_buf;
	_joined_buf = sbuf;

	return true;
}


void
ObjectBuffer::unjoin()
{
	_joined_buf = NULL;
	_buf = _local_buf;
}


void
ObjectBuffer::copy(Context& context, const Buffer* src_buf)
{
	const ObjectBuffer* src = dynamic_cast<const ObjectBuffer*>(src_buf);
	if (!src || src == this || src->_buf == _buf)
		return;

	// Copy if src is a POD object only, that fits
	if (src->_buf->type != 0 && src->_buf->size <= size())
		memcpy(_buf, src->_buf, sizeof(LV2_Object) + src->_buf->size);
}


void
ObjectBuffer::resize(size_t size)
{
	const bool     using_local_data = (_buf == _local_buf);
	const uint32_t contents_size    = sizeof(LV2_Object) + _buf->size;

	_local_buf = (LV2_Object*)realloc(_buf, sizeof(LV2_Object) + size);
	_size      = size;

	// If we shrunk and chopped the current contents, clear corrupt data
	if (size < contents_size)
		clear();

	if (using_local_data)
		_buf = _local_buf;
}


} // namespace Ingen

