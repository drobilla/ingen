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

#include "ingen/shared/URIs.hpp"
#include "raul/log.hpp"

#include "AudioBuffer.hpp"
#include "Buffer.hpp"
#include "Context.hpp"

namespace Ingen {
namespace Server {

static inline bool
is_audio(Shared::URIs& uris, LV2_URID type)
{
	return type == uris.atom_Float || type == uris.atom_Sound;
}

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

void
mix(Context&            context,
    Shared::URIs&       uris,
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
		std::cerr << "FIXME: event mix" << std::endl;
	}
#if 0
	case PortType::EVENTS:
		dst->clear();
		for (uint32_t i = 0; i < num_srcs; ++i) {
			assert(srcs[i]->type() == PortType::EVENTS);
			srcs[i]->rewind();
		}

		while (true) {
			const EventBuffer* first = NULL;
			for (uint32_t i = 0; i < num_srcs; ++i) {
				const EventBuffer* const src = (const EventBuffer*)srcs[i];
				if (src->is_valid()) {
					if (!first || src->get_event()->frames < first->get_event()->frames)
						first = src;
				}
			}
			if (first) {
				const LV2_Event* const ev = first->get_event();
				((EventBuffer*)dst)->append(
					ev->frames, ev->subframes, ev->type, ev->size,
					(const uint8_t*)ev + sizeof(LV2_Event));
				first->increment();
			} else {
				break;
			}
		}
		dst->rewind();
#endif
}

} // namespace Server
} // namespace Ingen
