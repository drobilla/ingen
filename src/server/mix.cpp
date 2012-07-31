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

#include "lv2/lv2plug.in/ns/ext/atom/util.h"

#include "ingen/URIs.hpp"
#include "raul/log.hpp"

#include "AudioBuffer.hpp"
#include "Buffer.hpp"
#include "Context.hpp"

namespace Ingen {
namespace Server {

static inline void
audio_accumulate(Context& context, AudioBuffer* dst, const AudioBuffer* src)
{
	Sample* const       dst_buf = dst->data();
	const Sample* const src_buf = src->data();

	if (dst->is_control()) {
		if (src->is_control()) {  // control => control
			dst_buf[0] += src_buf[0];
		} else {  // audio => control
			dst_buf[0] += src_buf[context.offset()];
		}
	} else {
		const SampleCount end = context.offset() + context.nframes();
		if (src->is_control()) {  // control => audio
			for (SampleCount i = context.offset(); i < end; ++i) {
				dst_buf[i] += src_buf[0];
			}
		} else {  // audio => audio
			for (SampleCount i = context.offset(); i < end; ++i) {
				dst_buf[i] += src_buf[i];
			}
		}
	}
}

static inline bool
is_audio(URIs& uris, LV2_URID type)
{
	return type == uris.atom_Float || type == uris.atom_Sound;
}

static inline bool
is_end(const Buffer* buf, LV2_Atom_Event* ev)
{
	return lv2_atom_sequence_is_end(
		(LV2_Atom_Sequence_Body*)LV2_ATOM_BODY(buf->atom()),
		buf->atom()->size,
		ev);
}

void
mix(Context&            context,
    URIs&               uris,
    Buffer*             dst,
    const Buffer*const* srcs,
    uint32_t            num_srcs)
{
	if (num_srcs == 1) {
		dst->copy(context, srcs[0]);
	} else if (is_audio(uris, dst->type())) {
		// Copy the first source
		dst->copy(context, srcs[0]);

		// Mix in the rest
		for (uint32_t i = 1; i < num_srcs; ++i) {
			assert(is_audio(uris, srcs[i]->type()));
			audio_accumulate(context,
			                 (AudioBuffer*)dst,
			                 (const AudioBuffer*)srcs[i]);
		}
	} else {
		assert(dst->type() == uris.atom_Sequence);
		LV2_Atom_Event* iters[num_srcs];
		for (uint32_t i = 0; i < num_srcs; ++i) {
			assert(srcs[i]->type() == uris.atom_Sequence);
			iters[i] = lv2_atom_sequence_begin(
				(LV2_Atom_Sequence_Body*)LV2_ATOM_BODY(srcs[i]->atom()));
			if (is_end(srcs[i], iters[i])) {
				iters[i] = NULL;
			}
		}

		while (true) {
			const LV2_Atom_Event* first   = NULL;
			uint32_t              first_i = 0;
			for (uint32_t i = 0; i < num_srcs; ++i) {
				const LV2_Atom_Event* const ev = iters[i];
				if (!first || (ev && ev->time.frames < first->time.frames)) {
					first   = ev;
					first_i = i;
				}
			}

			if (first) {
				dst->append_event(
					first->time.frames, first->body.size, first->body.type,
					(const uint8_t*)LV2_ATOM_BODY(&first->body));

				iters[first_i] = lv2_atom_sequence_next(first);
				if (is_end(srcs[first_i], iters[first_i])) {
					iters[first_i] = NULL;
				}
			} else {
				break;
			}
		}
	}
}

} // namespace Server
} // namespace Ingen
