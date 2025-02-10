/*
  This file is part of Ingen.
  Copyright 2007-2016 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "InputPort.hpp"

#include "ArcImpl.hpp"
#include "BlockImpl.hpp"
#include "Buffer.hpp"
#include "BufferFactory.hpp"
#include "BufferRef.hpp"
#include "GraphImpl.hpp"
#include "NodeImpl.hpp"
#include "PortType.hpp"
#include "RunContext.hpp"
#include "mix.hpp"

#include <ingen/Atom.hpp>
#include <ingen/Node.hpp>
#include <ingen/URIs.hpp>
#include <lv2/urid/urid.h>
#include <raul/Array.hpp>
#include <raul/Maid.hpp>

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <memory>

namespace ingen::server {

InputPort::InputPort(BufferFactory&      bufs,
                     BlockImpl*          parent,
                     const raul::Symbol& symbol,
                     uint32_t            index,
                     uint32_t            poly,
                     PortType            type,
                     LV2_URID            buffer_type,
                     const Atom&         value,
                     size_t              buffer_size)
	: PortImpl(bufs, parent, symbol, index, poly, type, buffer_type, value, buffer_size, false)
{
	const ingen::URIs& uris = bufs.uris();

	if (parent->graph_type() != Node::GraphType::GRAPH) {
		add_property(uris.rdf_type, uris.lv2_InputPort.urid_atom());
	}
}

bool
InputPort::apply_poly(RunContext& ctx, const uint32_t poly)
{
	const bool ret = PortImpl::apply_poly(ctx, poly);

	(void)ret;
	assert(_voices->size() >= (ret ? poly : 1));

	return true;
}

bool
InputPort::get_buffers(BufferFactory&                   bufs,
                       PortImpl::GetFn                  get,
                       const raul::managed_ptr<Voices>& voices,
                       uint32_t                         poly,
                       size_t                           num_in_arcs) const
{
	if (is_a(PortType::ATOM) && !_value.is_valid()) {
		poly = 1;
	}

	if (is_a(PortType::AUDIO) && num_in_arcs == 0) {
		// Audio input with no arcs, use shared zero buffer
		for (uint32_t v = 0; v < poly; ++v) {
			voices->at(v).buffer = bufs.silent_buffer();
		}
		return false;
	}

	// Otherwise, allocate local buffers
	for (uint32_t v = 0; v < poly; ++v) {
		voices->at(v).buffer.reset();
		voices->at(v).buffer = (bufs.*get)(
			buffer_type(), _value.type(), _buffer_size);
		voices->at(v).buffer->clear();
		if (_value.is_valid()) {
			voices->at(v).buffer->set_value(_value);
		}
	}
	return true;
}

bool
InputPort::pre_get_buffers(BufferFactory&             bufs,
                           raul::managed_ptr<Voices>& voices,
                           uint32_t                   poly) const
{
	return get_buffers(bufs, &BufferFactory::get_buffer, voices, poly, _num_arcs);
}

bool
InputPort::setup_buffers(RunContext& ctx, BufferFactory& bufs, uint32_t poly)
{
	if (is_a(PortType::ATOM) && !_value.is_valid()) {
		poly = 1;
	}

	if (_arcs.size() == 1 && !is_a(PortType::ATOM) && !_arcs.front().must_mix()) {
		// Single non-mixing connection, use buffers directly
		for (uint32_t v = 0; v < poly; ++v) {
			_voices->at(v).buffer = _arcs.front().buffer(ctx, v);
		}
		return false;
	}

	return get_buffers(bufs, &BufferFactory::claim_buffer, _voices, poly, _arcs.size());
}

void
InputPort::add_arc(RunContext&, ArcImpl& c)
{
	_arcs.push_front(c);
}

void
InputPort::remove_arc(ArcImpl& arc)
{
	_arcs.erase(_arcs.iterator_to(arc));
}

uint32_t
InputPort::max_tail_poly(RunContext&) const
{
	return parent_block()->parent_graph()->internal_poly_process();
}

void
InputPort::pre_process(RunContext& ctx)
{
	if (_arcs.empty()) {
		// No incoming arcs, just handle user-set value
		for (uint32_t v = 0; v < _poly; ++v) {
			// Update set state
			update_set_state(ctx, v);

			// Prepare for write in case a set event executes this cycle
			if (!_parent->is_main()) {
				buffer(v)->prepare_write(ctx);
			}
		}
	} else if (direct_connect()) {
		// Directly connected, use source's buffer directly
		for (uint32_t v = 0; v < _poly; ++v) {
			_voices->at(v).buffer = _arcs.front().buffer(ctx, v);
		}
	} else {
		// Mix down to local buffers in pre_run()
		for (uint32_t v = 0; v < _poly; ++v) {
			buffer(v)->prepare_write(ctx);
		}
	}
}

void
InputPort::pre_run(RunContext& ctx)
{
	if ((_user_buffer || !_arcs.empty()) && !direct_connect()) {
		const uint32_t src_poly   = max_tail_poly(ctx);
		const uint32_t max_n_srcs = (_arcs.size() * src_poly) + 1;

		for (uint32_t v = 0; v < _poly; ++v) {
			if (!buffer(v)->get<void>()) {
				continue;
			}

			// Get all sources for this voice
			const Buffer* srcs[max_n_srcs];
			uint32_t      n_srcs = 0;

			if (_user_buffer) {
				// Add buffer with user/UI input for this cycle
				srcs[n_srcs++] = _user_buffer.get();
			}

			for (const auto& arc : _arcs) {
				if (_poly == 1) {
					// P -> 1 or 1 -> 1: all tail voices => each head voice
					for (uint32_t w = 0; w < arc.tail()->poly(); ++w) {
						assert(n_srcs < max_n_srcs);
						srcs[n_srcs++] = arc.buffer(ctx, w).get();
						assert(srcs[n_srcs - 1]);
					}
				} else {
					// P -> P or 1 -> P: tail voice => corresponding head voice
					assert(n_srcs < max_n_srcs);
					srcs[n_srcs++] = arc.buffer(ctx, v).get();
					assert(srcs[n_srcs - 1]);
				}
			}

			// Then mix them into our buffer for this voice
			mix(ctx, buffer(v).get(), srcs, n_srcs);
			update_values(ctx.offset(), v);
		}
	} else if (is_a(PortType::CONTROL)) {
		for (uint32_t v = 0; v < _poly; ++v) {
			update_values(ctx.offset(), v);
		}
	}
}

SampleCount
InputPort::next_value_offset(SampleCount offset, SampleCount end) const
{
	SampleCount earliest = end;

	if (_user_buffer) {
		earliest = _user_buffer->next_value_offset(offset, end);
	}

	for (const auto& arc : _arcs) {
		const SampleCount o = arc.tail()->next_value_offset(offset, end);
		earliest = std::min(o, earliest);
	}

	return earliest;
}

void
InputPort::post_process(RunContext& ctx)
{
	if (!_arcs.empty() || _force_monitor_update) {
		monitor(ctx, _force_monitor_update);
		_force_monitor_update = false;
	}

	/* Finished processing any user/UI messages for this cycle, drop reference
	   to user buffer. */
	_user_buffer.reset();
}

bool
InputPort::direct_connect() const
{
	return _arcs.size() == 1
		&& !_parent->is_main()
		&& !_arcs.front().must_mix()
		&& buffer(0)->type() != _bufs.uris().atom_Sequence;
}

} // namespace ingen::server
