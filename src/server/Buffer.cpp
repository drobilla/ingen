/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#define __STDC_LIMIT_MACROS 1

#include <stdint.h>
#include <string.h>

#include <algorithm>

#include "ingen/shared/LV2Features.hpp"
#include "ingen/shared/URIMap.hpp"
#include "ingen/shared/URIs.hpp"
#include "ingen/shared/World.hpp"
#include "ingen_config.h"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "raul/log.hpp"

#include "Buffer.hpp"
#include "BufferFactory.hpp"
#include "Engine.hpp"

namespace Ingen {
namespace Server {

Buffer::Buffer(BufferFactory& bufs, LV2_URID type, uint32_t capacity)
	: _factory(bufs)
	, _type(type)
	, _capacity(capacity)
	, _next(NULL)
	, _refs(0)
{
	if (capacity > UINT32_MAX) {
		Raul::error << "Event buffer size " << capacity << " too large, aborting."
		            << std::endl;
		throw std::bad_alloc();
	}

#ifdef HAVE_POSIX_MEMALIGN
	int ret = posix_memalign((void**)&_atom, 16, capacity);
#else
	_atom = (LV2_Atom*)malloc(capacity);
	int ret = (_atom != NULL) ? 0 : -1;
#endif

	if (ret != 0) {
		Raul::error << "Failed to allocate event buffer." << std::endl;
		throw std::bad_alloc();
	}

	memset(_atom, 0, capacity);
	_atom->type = type;
	assert(_atom->type != 1);

	clear();
}

Buffer::~Buffer()
{
	free(_atom);
}

void
Buffer::recycle()
{
	_factory.recycle(this);
}

void
Buffer::clear()
{
	_atom->size = 0;
}

void
Buffer::copy(Context& context, const Buffer* src)
{
	// Copy only if src is a POD object that fits
	if (src->_atom->type != 0 && sizeof(LV2_Atom) + src->_atom->size <= capacity()) {
		memcpy(_atom, src->_atom, sizeof(LV2_Atom) + src->_atom->size);
	}
	assert(_atom->type != 1);
}

void
Buffer::resize(uint32_t capacity)
{
	_atom     = (LV2_Atom*)realloc(_atom, capacity);
	_capacity = capacity;
	clear();
}

void*
Buffer::port_data(PortType port_type, SampleCount offset)
{
	switch (port_type.symbol()) {
	case PortType::CONTROL:
	case PortType::CV:
	case PortType::AUDIO:
		assert(_atom->type == _type);
		if (_atom->type == _factory.uris().atom_Float) {
			return (float*)LV2_ATOM_BODY(_atom);
		} else if (_atom->type == _factory.uris().atom_Sound) {
			return (float*)LV2_ATOM_CONTENTS(LV2_Atom_Vector, _atom) + offset;
		} else {
			Raul::warn << "Audio data requested from non-audio buffer " << this << " :: "
			           << _atom->type << " - "
			           << _factory.engine().world()->uri_map().unmap_uri(_atom->type)
			           << std::endl;
			assert(false);
			return NULL;
		}
		break;
	default:
		return _atom;
	}
}

const void*
Buffer::port_data(PortType port_type, SampleCount offset) const
{
	return const_cast<void*>(
		const_cast<Buffer*>(this)->port_data(port_type, offset));
}

void
Buffer::prepare_write(Context& context)
{
	if (_type == _factory.uris().atom_Sequence) {
		_atom->size = sizeof(LV2_Atom_Sequence_Body);
	}
}

bool
Buffer::append_event(int64_t        frames,
                     uint32_t       size,
                     uint32_t       type,
                     const uint8_t* data)
{
	if (sizeof(LV2_Atom) + _atom->size + lv2_atom_pad_size(size) > _capacity) {
		return false;
	}

	LV2_Atom_Sequence* seq = (LV2_Atom_Sequence*)_atom;
	LV2_Atom_Event*    ev  = (LV2_Atom_Event*)(
		(uint8_t*)seq + lv2_atom_total_size(&seq->atom));

	ev->time.frames = frames;
	ev->body.size   = size;
	ev->body.type   = type;
	memcpy(ev + 1, data, size);

	_atom->size += sizeof(LV2_Atom_Event) + lv2_atom_pad_size(size);

	return true;
}

} // namespace Server
} // namespace Ingen
