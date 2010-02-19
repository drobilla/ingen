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
#include "raul/log.hpp"
#include "uri-map.lv2/uri-map.h"
#include "ingen-config.h"
#include "shared/LV2Features.hpp"
#include "shared/LV2URIMap.hpp"
#include "ObjectBuffer.hpp"
#include "Engine.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {

using namespace Shared;


/** Allocate a new object buffer.
 * \a capacity is in bytes, including LV2_Object header
 */
ObjectBuffer::ObjectBuffer(BufferFactory& factory, size_t capacity)
	: Buffer(factory, PortType(PortType::VALUE), capacity)
{
	capacity += sizeof(LV2_Object);

#ifdef HAVE_POSIX_MEMALIGN
	const int ret = posix_memalign((void**)&_buf, 16, capacity);
#else
	_buf = (LV2_Object*)malloc(capacity);
	const int ret = (_buf != NULL) ? 0 : -1;
#endif

	if (ret != 0) {
		error << "Failed to allocate object buffer.  Aborting." << endl;
		exit(EXIT_FAILURE);
	}

	clear();
}


void
ObjectBuffer::clear()
{
	// null
	_buf->type = 0;
	_buf->size = 0;
}


void
ObjectBuffer::copy(Context& context, const Buffer* src_buf)
{
	const ObjectBuffer* src = dynamic_cast<const ObjectBuffer*>(src_buf);
	if (!src || src == this || src->_buf == _buf)
		return;

	// Copy only if src is a POD object that fits
	if (src->_buf->type != 0 && src_buf->size() <= size())
		memcpy(_buf, src->_buf, sizeof(LV2_Object) + src_buf->size());
}


void
ObjectBuffer::resize(size_t size)
{
	const uint32_t contents_size = sizeof(LV2_Object) + _buf->size;

	_buf  = (LV2_Object*)realloc(_buf, sizeof(LV2_Object) + size);
	_size = size + sizeof(LV2_Object);

	// If we shrunk and chopped the current contents, clear corrupt data
	if (size < contents_size)
		clear();
}


void*
ObjectBuffer::port_data(PortType port_type)
{
	switch (port_type.symbol()) {
	case PortType::CONTROL:
	case PortType::AUDIO:
		switch (_type.symbol()) {
			case PortType::CONTROL:
				return (float*)object()->body;
			case PortType::AUDIO:
				return ((LV2_Vector_Body*)object()->body)->elems;
			default:
				warn << "Audio data requested from non-audio buffer" << endl;
				return NULL;
		}
		break;
	default:
		return _buf;
	}
}


const void*
ObjectBuffer::port_data(PortType port_type) const
{
	switch (port_type.symbol()) {
	case PortType::CONTROL:
		return _buf + sizeof(LV2_Object);
	case PortType::AUDIO:
		return _buf + sizeof(LV2_Object) + sizeof(LV2_Vector_Body);
	default:
		return _buf;
	}
}


void
ObjectBuffer::prepare_write(Context& context)
{
	_buf->size = _size - sizeof(LV2_Object);
}


} // namespace Ingen

