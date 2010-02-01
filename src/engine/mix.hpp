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

#ifndef INGEN_ENGINE_MIX_HPP
#define INGEN_ENGINE_MIX_HPP

#include "raul/log.hpp"
#include "interface/PortType.hpp"
#include "Buffer.hpp"
#include "Context.hpp"

using namespace Raul;

namespace Ingen {

inline void
mix(Context& context, Buffer* dst, const Buffer*const* srcs, uint32_t num_srcs)
{
	using Shared::PortType;
	switch (dst->type().symbol()) {
	case PortType::AUDIO:
	case PortType::CONTROL:
		// Copy the first source
		dst->copy(context, srcs[0]);

		// Mix in the rest
		for (uint32_t i = 1; i < num_srcs; ++i) {
			assert(srcs[i]->type() == PortType::AUDIO || srcs[i]->type() == PortType::CONTROL);
			((AudioBuffer*)dst)->accumulate(context, (AudioBuffer*)srcs[i]);
		}

		break;

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
		break;

	default:
		error << "Mix of unsupported buffer types" << std::endl;
		return;
	}
}

} // namespace Ingen

#endif // INGEN_ENGINE_MIX_HPP
