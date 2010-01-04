/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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
#include <cassert>
#include <stdlib.h>
#include "raul/SharedPtr.hpp"
#include "object.lv2/object.h"
#include "ingen-config.h"
#include "AudioBuffer.hpp"
#include "ProcessContext.hpp"
#include "LV2Features.hpp"
#include "LV2URIMap.hpp"

using namespace std;

/* TODO: Be sure these functions are vectorized by GCC when its vectorizer
 * stops sucking.  Probably a good idea to inline them as well */

namespace Ingen {

using namespace Shared;


AudioBuffer::AudioBuffer(Shared::PortType type, size_t size)
	: ObjectBuffer(size + sizeof(LV2_Object)
			+ (type == PortType::AUDIO ? sizeof(LV2_Vector_Body) : 0))
	, _state(OK)
	, _set_value(0)
	, _set_time(0)
{
	assert(size >= sizeof(Sample));
	assert(this->size() > size);
	assert(data());
	_type = type;

	// Control port / Single float object
	if (type == PortType::CONTROL) {
		object()->type = 0;//map->float_type;

	// Audio port / Vector of float
	} else {
		assert(type == PortType::AUDIO);
		object()->type = 0;//map->vector_type;
		LV2_Vector_Body* body = (LV2_Vector_Body*)object()->body;
		body->elem_count = size / sizeof(Sample);
		body->elem_type = 0;//map->float_type;
	}

	/*cout << "Created Audio Buffer" << endl
		<< "\tobject @ " << (void*)object() << endl
		<< "\tbody @   " << (void*)object()->body
		<< "\t(offset " << (char*)object()->body - (char*)object() << ")" << endl
		<< "\tdata @   " << (void*)data()
		<< "\t(offset " << (char*)data() - (char*)object() << ")"
		<< endl;*/
}


void
AudioBuffer::resize(size_t size)
{
	if (_type == PortType::AUDIO) {
		ObjectBuffer::resize(size + sizeof(LV2_Vector_Body));
		vector()->elem_count = size / sizeof(Sample);
	}
	clear();
}


/** Empty (ie zero) the buffer.
 */
void
AudioBuffer::clear()
{
	assert(nframes() != 0);
	set_block(0, 0, nframes() - 1);
	_state = OK;
}


/** Set value of buffer to @a val after @a start_sample.
 *
 * The Buffer will handle setting the intial portion of the buffer to the
 * value on the next cycle automatically (if @a start_sample is > 0), as
 * long as pre_process() is called every cycle.
 */
void
AudioBuffer::set_value(Sample val, FrameTime cycle_start, FrameTime time)
{
	if (is_control())
		time = cycle_start;

	const FrameTime offset = time - cycle_start;
	assert(nframes() != 0);
	assert(offset <= nframes());

	if (offset < nframes()) {
		set_block(val, offset, nframes() - 1);

		if (offset > 0)
			_state = HALF_SET_CYCLE_1;
	} // else trigger at very end of block

	_set_time = time;
	_set_value = val;
}


/** Set a block of buffer to @a val.
 *
 * @a start_sample and @a end_sample define the inclusive range to be set.
 */
void
AudioBuffer::set_block(Sample val, size_t start_offset, size_t end_offset)
{
	assert(end_offset >= start_offset);
	assert(end_offset < nframes());

	Sample* const buf = data();
	assert(buf);

	for (size_t i = start_offset; i <= end_offset; ++i)
		buf[i] = val;
}


/** Copy a block of @a src into buffer.
 *
 * @a start_sample and @a end_sample define the inclusive range to be set.
 * This function only copies the same range in one buffer to another.
 */
void
AudioBuffer::copy(const Sample* src, size_t start_sample, size_t end_sample)
{
	assert(end_sample >= start_sample);
	assert(nframes() != 0);

	Sample* const buf = data();
	assert(buf);

	const size_t copy_end = std::min(end_sample, (size_t)nframes() - 1);
	for (size_t i = start_sample; i <= copy_end; ++i)
		buf[i] = src[i];
}


void
AudioBuffer::copy(Context& context, const Buffer* src)
{
	const AudioBuffer* src_abuf = dynamic_cast<const AudioBuffer*>(src);
	if (!src_abuf) {
		clear();
		return;
	}

	// Control => Control or Audio => Audio
	if (src_abuf->is_control() == is_control()) {
		ObjectBuffer::copy(context, src);

	// Control => Audio or Audio => Control
	} else {
		set_block(src_abuf->data()[0], 0, nframes());
	}
}


/** Accumulate a block of @a src into buffer.
 *
 * @a start_sample and @a end_sample define the inclusive range to be accumulated.
 * This function only adds the same range in one buffer to another.
 */
void
AudioBuffer::accumulate(Context& context, const AudioBuffer* const src)
{
	Sample* const       buf     = data();
	const Sample* const src_buf = src->data();

	const size_t frames = std::min(nframes(), src->nframes());
	assert(frames != 0);

	// Mix initial portions
	SampleCount i = 0;
	for (; i < frames; ++i)
		buf[i] += src_buf[i];

	// Extend/Mix the final sample of src if it is shorter
	const Sample last = src_buf[i - 1];
	while (i < nframes())
		buf[i++] += last;
}


void
AudioBuffer::prepare_read(Context& context)
{
	assert(nframes() != 0);
	switch (_state) {
	case HALF_SET_CYCLE_1:
		if (context.start() > _set_time)
			_state = HALF_SET_CYCLE_2;
		break;
	case HALF_SET_CYCLE_2:
		set_block(_set_value, 0, nframes() - 1);
		_state = OK;
		break;
	default:
		break;
	}
}


} // namespace Ingen
