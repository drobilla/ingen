/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "mix.hpp"

#include "Buffer.hpp"
#include "RunContext.hpp"
#include "types.hpp"

#include <lv2/atom/atom.h>
#include <lv2/atom/util.h>

namespace ingen::server {

static inline bool
is_end(const Buffer* buf, const LV2_Atom_Event* ev)
{
	const auto* atom = buf->get<const LV2_Atom>();
	return lv2_atom_sequence_is_end(
		static_cast<const LV2_Atom_Sequence_Body*>(LV2_ATOM_BODY_CONST(atom)),
		atom->size,
		ev);
}

void
mix(const RunContext&   ctx,
    Buffer*             dst,
    const Buffer*const* srcs,
    uint32_t            num_srcs)
{
	if (num_srcs == 1) {
		dst->copy(ctx, srcs[0]);
	} else if (dst->is_control()) {
		Sample* const out = dst->samples();
		out[0] = srcs[0]->value_at(0);
		for (uint32_t i = 1; i < num_srcs; ++i) {
			out[0] += srcs[i]->value_at(0);
		}
	} else if (dst->is_audio()) {
		// Copy the first source
		dst->copy(ctx, srcs[0]);

		// Mix in the rest
		Sample* __restrict const out = dst->samples();
		const SampleCount        end = ctx.nframes();
		for (uint32_t i = 1; i < num_srcs; ++i) {
			const Sample* __restrict const in = srcs[i]->samples();
			if (srcs[i]->is_control()) { // control => audio
				for (SampleCount j = 0; j < end; ++j) {
					out[j] += in[0];
				}
			} else if (srcs[i]->is_audio()) { // audio => audio
				for (SampleCount j = 0; j < end; ++j) {
					out[j] += in[j];
				}
			} else if (srcs[i]->is_sequence()) { // sequence => audio
				dst->render_sequence(ctx, srcs[i], true);
			}
		}
	} else if (dst->is_sequence()) {
		const LV2_Atom_Event* iters[num_srcs];
		for (uint32_t i = 0; i < num_srcs; ++i) {
			iters[i] = nullptr;
			if (srcs[i]->is_sequence()) {
				const auto* seq = srcs[i]->get<const LV2_Atom_Sequence>();
				iters[i] = lv2_atom_sequence_begin(&seq->body);
				if (is_end(srcs[i], iters[i])) {
					iters[i] = nullptr;
				}
			}
		}

		while (true) {
			const LV2_Atom_Event* first   = nullptr;
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
					static_cast<const uint8_t*>(LV2_ATOM_BODY_CONST(&first->body)));

				iters[first_i] = lv2_atom_sequence_next(first);
				if (is_end(srcs[first_i], iters[first_i])) {
					iters[first_i] = nullptr;
				}
			} else {
				break;
			}
		}
	}
}

} // namespace ingen::server
