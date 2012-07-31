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

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#include <new>

#include "ingen/URIMap.hpp"
#include "ingen/URIs.hpp"
#include "ingen/World.hpp"
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
#ifdef HAVE_POSIX_MEMALIGN
	int ret = posix_memalign((void**)&_atom, 16, capacity);
#else
	_atom = (LV2_Atom*)malloc(capacity);
	int ret = (_atom != NULL) ? 0 : -1;
#endif

	if (ret) {
		Raul::error << "Failed to allocate event buffer." << std::endl;
		throw std::bad_alloc();
	}

	memset(_atom, 0, capacity);
	_atom->size = capacity - sizeof(LV2_Atom);
	_atom->type = type;

	if (type == bufs.uris().atom_Sound) {
		// Audio port (Vector of float)
		LV2_Atom_Vector* vec = (LV2_Atom_Vector*)_atom;
		vec->body.child_size = sizeof(float);
		vec->body.child_type = bufs.uris().atom_Float;
	}

	clear();
}

Buffer::~Buffer()
{
	free(_atom);
}

bool
Buffer::is_audio() const
{
	return _type == _factory.uris().atom_Sound;
}

bool
Buffer::is_control() const
{
	return _type == _factory.uris().atom_Float;
}

bool
Buffer::is_sequence() const
{
	return _type == _factory.uris().atom_Sequence;
}

void
Buffer::recycle()
{
	_factory.recycle(this);
}

void
Buffer::clear()
{
	if (is_audio() || is_control()) {
		_atom->size = _capacity - sizeof(LV2_Atom);
		set_block(0, 0, nframes() - 1);
	} else if (is_sequence()) {
		_atom->size = sizeof(LV2_Atom_Sequence_Body);
	}
}

void
Buffer::copy(Context& context, const Buffer* src)
{
	if (_type == src->type() && src->_atom->size + sizeof(LV2_Atom) <= _capacity) {
		memcpy(_atom, src->_atom, sizeof(LV2_Atom) + src->_atom->size);
	} else if (src->is_audio() && is_control()) {
		samples()[0] = src->samples()[context.offset()];
	} else if (src->is_control() && is_audio()) {
		samples()[context.offset()] = src->samples()[0];
	} else {
		clear();
	}
}

void
Buffer::set_block(Sample val, size_t start_offset, size_t end_offset)
{
	assert(end_offset >= start_offset);
	assert(end_offset < nframes());

	Sample* const buf = samples();
	for (size_t i = start_offset; i <= end_offset; ++i) {
		buf[i] = val;
	}
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

float
Buffer::peak(const Context& context) const
{
	float               peak = 0.0f;
	const Sample* const buf  = samples();
	for (FrameTime i = 0; i < context.nframes(); ++i) {
		peak = fmaxf(peak, buf[i]);
	}
	return peak;
}

void
Buffer::prepare_write(Context& context)
{
	if (_type == _factory.uris().atom_Sequence) {
		_atom->size = sizeof(LV2_Atom_Sequence_Body);
	}
}

void
Buffer::prepare_output_write(Context& context)
{
	if (_type == _factory.uris().atom_Sequence) {
		_atom->type = (LV2_URID)_factory.uris().atom_Chunk;
		_atom->size = _capacity - sizeof(LV2_Atom_Sequence);
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
