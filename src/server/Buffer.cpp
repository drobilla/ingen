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

#include <math.h>
#include <new>
#include <stdint.h>
#include <string.h>

#ifdef __SSE__
#    include <xmmintrin.h>
#endif

#include "ingen/URIMap.hpp"
#include "ingen/URIs.hpp"
#include "ingen/World.hpp"
#include "ingen_config.h"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "ingen/Log.hpp"

#include "Buffer.hpp"
#include "BufferFactory.hpp"
#include "Engine.hpp"

namespace Ingen {
namespace Server {

Buffer::Buffer(BufferFactory& bufs,
               LV2_URID       type,
               LV2_URID       value_type,
               uint32_t       capacity)
	: _factory(bufs)
	, _type(type)
	, _value_type(value_type)
	, _capacity(capacity)
	, _latest_event(0)
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
		bufs.engine().log().error("Failed to allocate event buffer\n");
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

	if (type == bufs.uris().atom_Sequence && value_type) {
		_value_buffer = bufs.get_buffer(value_type, 0, 0, false);
	}

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
Buffer::set_type(LV2_URID type, LV2_URID value_type)
{
	_type = type;
	if (type == _factory.uris().atom_Sequence && value_type) {
		_value_buffer = _factory.get_buffer(value_type, 0, 0, false);
	}
}

void
Buffer::clear()
{
	if (is_audio() || is_control()) {
		_atom->size = _capacity - sizeof(LV2_Atom);
		set_block(0, 0, nframes());
	} else if (is_sequence()) {
		LV2_Atom_Sequence* seq = (LV2_Atom_Sequence*)_atom;
		_atom->type    = _factory.uris().atom_Sequence;
		_atom->size    = sizeof(LV2_Atom_Sequence_Body);
		seq->body.unit = 0;
		seq->body.pad  = 0;
		_latest_event  = 0;
	}
}

void
Buffer::render_sequence(const Context& context, const Buffer* src, bool add)
{
	const LV2_URID           atom_Float = _factory.uris().atom_Float;
	const LV2_Atom_Sequence* seq        = (const LV2_Atom_Sequence*)src->atom();
	const LV2_Atom_Float*    init       = (const LV2_Atom_Float*)src->value();
	float                    value      = init ? init->body : 0.0f;
	SampleCount              offset     = context.offset();
	LV2_ATOM_SEQUENCE_FOREACH(seq, ev) {
		if (ev->time.frames >= offset && ev->body.type == atom_Float) {
			write_block(value, offset, ev->time.frames, add);
			value  = ((const LV2_Atom_Float*)&ev->body)->body;
			offset = ev->time.frames;
		}
	}
	write_block(value, offset, context.offset() + context.nframes(), add);
}

void
Buffer::copy(const Context& context, const Buffer* src)
{
	if (_type == src->type() && src->_atom->size + sizeof(LV2_Atom) <= _capacity) {
		memcpy(_atom, src->_atom, sizeof(LV2_Atom) + src->_atom->size);
		if (value() && src->value()) {
			memcpy(value(), src->value(), lv2_atom_total_size(src->value()));
		}
	} else if (src->is_audio() && is_control()) {
		samples()[0] = src->samples()[0];
	} else if (src->is_control() && is_audio()) {
		set_block(src->samples()[0], 0, context.nframes());
	} else if (src->is_sequence() && is_audio() &&
	           src->value_type() == _factory.uris().atom_Float) {
		render_sequence(context, src, false);
	} else {
		clear();
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
	switch (port_type.id()) {
	case PortType::ID::CONTROL:
	case PortType::ID::CV:
	case PortType::ID::AUDIO:
		if (_atom->type == _factory.uris().atom_Float) {
			return (float*)LV2_ATOM_BODY(_atom);
		} else if (_atom->type == _factory.uris().atom_Sound) {
			return (float*)LV2_ATOM_CONTENTS(LV2_Atom_Vector, _atom) + offset;
		}
		break;
	default:
		return _atom;
	}
	return NULL;
}

const void*
Buffer::port_data(PortType port_type, SampleCount offset) const
{
	return const_cast<void*>(
		const_cast<Buffer*>(this)->port_data(port_type, offset));
}

#ifdef __SSE__
/** Vector fabsf */
static inline __m128
mm_abs_ps(__m128 x)
{
	const __m128 sign_mask = _mm_set1_ps(-0.0f);  // -0.0f = 1 << 31
	return _mm_andnot_ps(sign_mask, x);
}
#endif

float
Buffer::peak(const Context& context) const
{
#ifdef __SSE__
	const __m128* const vbuf    = (const __m128* const)samples();
	__m128              vpeak   = mm_abs_ps(vbuf[0]);
	const SampleCount   nblocks = context.nframes() / 4;

	// First, find the vector absolute max of the buffer
	for (SampleCount i = 1; i < nblocks; ++i) {
		vpeak = _mm_max_ps(vpeak, mm_abs_ps(vbuf[i]));
	}

	// Now we need the single max of vpeak
	// vpeak = ABCD
	// tmp   = CDAB
	__m128 tmp = _mm_shuffle_ps(vpeak, vpeak, _MM_SHUFFLE(2, 3, 0, 1));

	// vpeak = MAX(A,C) MAX(B,D) MAX(C,A) MAX(D,B)
	vpeak = _mm_max_ps(vpeak, tmp);

	// tmp = BADC of the new vpeak
	// tmp = MAX(B,D) MAX(A,C) MAX(D,B) MAX(C,A)
	tmp = _mm_shuffle_ps(vpeak, vpeak, _MM_SHUFFLE(1, 0, 3, 2));

	// vpeak = MAX(MAX(A,C), MAX(B,D)), ...
	vpeak = _mm_max_ps(vpeak, tmp);

	// peak = vpeak[0]
	float peak;
	_mm_store_ss(&peak, vpeak);

	return peak;
#else
	const Sample* const buf = samples();
	float peak = 0.0f;
	for (SampleCount i = 0; i < context.nframes(); ++i) {
		peak = fmaxf(peak, fabsf(buf[i]));
	}
	return peak;
#endif
}

void
Buffer::prepare_write(Context& context)
{
	if (_type == _factory.uris().atom_Sequence) {
		_atom->type   = (LV2_URID)_factory.uris().atom_Sequence;
		_atom->size   = sizeof(LV2_Atom_Sequence_Body);
		_latest_event = 0;
	}
}

void
Buffer::prepare_output_write(Context& context)
{
	if (_type == _factory.uris().atom_Sequence) {
		_atom->type   = (LV2_URID)_factory.uris().atom_Chunk;
		_atom->size   = _capacity - sizeof(LV2_Atom);
		_latest_event = 0;
	}
}

bool
Buffer::append_event(int64_t        frames,
                     uint32_t       size,
                     uint32_t       type,
                     const uint8_t* data)
{
	assert(frames >= _latest_event);
	if (_atom->type == _factory.uris().atom_Chunk) {
		// Chunk initialized with prepare_output_write(), clear
		clear();
	}

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

	_latest_event = frames;

	return true;
}

SampleCount
Buffer::next_value_offset(SampleCount offset, SampleCount end) const
{
	SampleCount earliest = end;
	LV2_ATOM_SEQUENCE_FOREACH((LV2_Atom_Sequence*)_atom, ev) {
		if (ev->time.frames >  offset   &&
		    ev->time.frames <  earliest &&
		    ev->body.type   == _value_type) {
			earliest = ev->time.frames;
			break;
		}
	}
	return earliest;
}

const LV2_Atom*
Buffer::value() const
{
	return _value_buffer ? _value_buffer->atom() : NULL;
}

LV2_Atom*
Buffer::value()
{
	return _value_buffer ? _value_buffer->atom() : NULL;
}

void
Buffer::update_value_buffer(SampleCount offset)
{
	if (!_value_buffer) {
		return;
	}

	LV2_ATOM_SEQUENCE_FOREACH((LV2_Atom_Sequence*)_atom, ev) {
		if (ev->time.frames <= offset && ev->body.type == _value_type) {
			memcpy(_value_buffer->atom(),
			       &ev->body,
			       lv2_atom_total_size(&ev->body));
			break;
		}
	}
}

void
intrusive_ptr_add_ref(Buffer* b)
{
	b->ref();
}

void
intrusive_ptr_release(Buffer* b)
{
	b->deref();
}

} // namespace Server
} // namespace Ingen
