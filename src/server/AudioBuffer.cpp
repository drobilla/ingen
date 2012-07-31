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

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"

#include "AudioBuffer.hpp"

using namespace std;

/* TODO: Be sure these functions are vectorized by GCC when its vectorizer
 * stops sucking.  Probably a good idea to inline them as well */

namespace Ingen {
namespace Server {

AudioBuffer::AudioBuffer(BufferFactory& bufs, LV2_URID type, uint32_t size)
	: Buffer(bufs, type, size)
{
	assert(size >= sizeof(LV2_Atom) + sizeof(Sample));
	assert(this->capacity() >= size);
	assert(data());

	if (type == bufs.uris().atom_Sound) {
		// Audio port (Vector of float)
		LV2_Atom_Vector* vec = (LV2_Atom_Vector*)_atom;
		vec->body.child_size = sizeof(float);
		vec->body.child_type = bufs.uris().atom_Float;
	}

	_atom->size = size - sizeof(LV2_Atom);
	_atom->type = type;

	clear();
}

/** Empty (ie zero) the buffer.
 */
void
AudioBuffer::clear()
{
	assert(nframes() != 0);
	set_block(0, 0, nframes() - 1);
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

	if (src_abuf->is_control() == is_control()) {
		// Rates match, direct copy
		Buffer::copy(context, src);
	} else if (!src_abuf->is_control() && is_control()) {
		// Audio => Control
		data()[0] = src_abuf->data()[context.offset()];
	} else if (src_abuf->is_control() && !is_control()) {
		// Control => Audio
		data()[context.offset()] = src_abuf->data()[0];
	}
}

float
AudioBuffer::peak(Context& context) const
{
	float peak = 0.0f;
	// FIXME: use context time range?
	for (FrameTime i = 0; i < nframes(); ++i) {
		peak = fmaxf(peak, value_at(i));
	}
	return peak;
}

} // namespace Server
} // namespace Ingen
