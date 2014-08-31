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

#include <cstdlib>
#include <cassert>

#include "ingen/Log.hpp"
#include "ingen/URIs.hpp"

#include "ArcImpl.hpp"
#include "BlockImpl.hpp"
#include "Buffer.hpp"
#include "BufferFactory.hpp"
#include "Engine.hpp"
#include "GraphImpl.hpp"
#include "InputPort.hpp"
#include "OutputPort.hpp"
#include "ProcessContext.hpp"
#include "mix.hpp"

using namespace std;

namespace Ingen {
namespace Server {

InputPort::InputPort(BufferFactory&      bufs,
                     BlockImpl*          parent,
                     const Raul::Symbol& symbol,
                     uint32_t            index,
                     uint32_t            poly,
                     PortType            type,
                     LV2_URID            buffer_type,
                     const Atom&         value,
                     size_t              buffer_size)
	: PortImpl(bufs, parent, symbol, index, poly, type, buffer_type, value, buffer_size)
	, _num_arcs(0)
{
	const Ingen::URIs& uris = bufs.uris();

	if (parent->graph_type() != Node::GraphType::GRAPH) {
		add_property(uris.rdf_type, uris.lv2_InputPort);
	}
}

bool
InputPort::apply_poly(ProcessContext& context, Raul::Maid& maid, uint32_t poly)
{
	bool ret = PortImpl::apply_poly(context, maid, poly);
	if (!ret)
		poly = 1;

	assert(_voices->size() >= poly);

	return true;
}

bool
InputPort::get_buffers(BufferFactory&      bufs,
                       Raul::Array<Voice>* voices,
                       uint32_t            poly,
                       bool                real_time) const
{
	const size_t num_arcs = real_time ? _arcs.size() : _num_arcs;

	if (is_a(PortType::ATOM) && !_value.is_valid()) {
		poly = 1;
	}

	if (is_a(PortType::AUDIO) && num_arcs == 0) {
		// Audio input with no arcs, use shared zero buffer
		for (uint32_t v = 0; v < poly; ++v) {
			voices->at(v).buffer = bufs.silent_buffer();
		}
		return false;

	} else if (num_arcs == 1) {
		if (real_time) {
			if (!_arcs.front().must_mix()) {
				// Single non-mixing connection, use buffers directly
				for (uint32_t v = 0; v < poly; ++v) {
					voices->at(v).buffer = _arcs.front().buffer(v);
				}
				return false;
			}
		}
	}

	// Otherwise, allocate local buffers
	for (uint32_t v = 0; v < poly; ++v) {
		voices->at(v).buffer.reset();
		voices->at(v).buffer = bufs.get_buffer(
			buffer_type(), _value.type(), _buffer_size, real_time);
		voices->at(v).buffer->clear();
	}
	return true;
}

void
InputPort::add_arc(ProcessContext& context, ArcImpl* c)
{
	_arcs.push_front(*c);
}

ArcImpl*
InputPort::remove_arc(ProcessContext& context, const OutputPort* tail)
{
	ArcImpl* arc = NULL;
	for (Arcs::iterator i = _arcs.begin(); i != _arcs.end(); ++i) {
		if (i->tail() == tail) {
			arc = &*i;
			_arcs.erase(i);
			break;
		}
	}

	if (!arc) {
		context.engine().log().error("Attempt to remove non-existent arc\n");
		return NULL;
	}

	return arc;
}

uint32_t
InputPort::max_tail_poly(Context& context) const
{
	return parent_block()->parent_graph()->internal_poly_process();
}

void
InputPort::pre_process(Context& context)
{
	if (_set_by_user) {
		// Value has been set (e.g. events pushed) by the user, don't smash it
		for (uint32_t v = 0; v < _poly; ++v) {
			buffer(v)->update_value_buffer(context.offset());
		}
		return;
	}

	if (_arcs.empty()) {
		for (uint32_t v = 0; v < _poly; ++v) {
			update_set_state(context, v);
		}
	} else if (direct_connect()) {
		for (uint32_t v = 0; v < _poly; ++v) {
			_voices->at(v).buffer = _arcs.front().buffer(v);
		}
	} else {
		// Mix down to local buffers in pre_run()
		for (uint32_t v = 0; v < _poly; ++v) {
			buffer(v)->prepare_write(context);
		}
	}
}

void
InputPort::pre_run(Context& context)
{
	if (!_set_by_user && !_arcs.empty() && !direct_connect()) {
		const uint32_t src_poly   = max_tail_poly(context);
		const uint32_t max_n_srcs = _arcs.size() * src_poly;

		for (uint32_t v = 0; v < _poly; ++v) {
			// Get all sources for this voice
			const Buffer* srcs[max_n_srcs];
			uint32_t      n_srcs = 0;
			for (const auto& arc : _arcs) {
				if (_poly == 1) {
					// P -> 1 or 1 -> 1: all tail voices => each head voice
					for (uint32_t w = 0; w < arc.tail()->poly(); ++w) {
						assert(n_srcs < max_n_srcs);
						srcs[n_srcs++] = arc.buffer(w, context.offset()).get();
						assert(srcs[n_srcs - 1]);
					}
				} else {
					// P -> P or 1 -> P: tail voice => corresponding head voice
					assert(n_srcs < max_n_srcs);
					srcs[n_srcs++] = arc.buffer(v, context.offset()).get();
					assert(srcs[n_srcs - 1]);
				}
			}

			// Then mix them into our buffer for this voice
			mix(context, buffer(v).get(), srcs, n_srcs);
		}
	}
}

SampleCount
InputPort::next_value_offset(SampleCount offset, SampleCount end) const
{
	SampleCount earliest = end;
	for (const auto& arc : _arcs) {
		if (arc.tail()->type() != this->type()) {
			const SampleCount o = arc.tail()->next_value_offset(offset, end);
			if (o < earliest) {
				earliest = o;
			}
		}
	}
	return earliest;
}

void
InputPort::update_values(SampleCount offset)
{
}

void
InputPort::post_process(Context& context)
{
	if (!_arcs.empty() || _force_monitor_update) {
		monitor(context, _force_monitor_update);
		_force_monitor_update = false;
	}

	if (_set_by_user) {
		if (_buffer_type == _bufs.uris().atom_Sequence) {
			// Clear events received via a SetPortValue
			for (uint32_t v = 0; v < _poly; ++v) {
				buffer(v)->prepare_write(context);
			}
		}
		_set_by_user = false;
	}
}

bool
InputPort::direct_connect() const
{
	return _arcs.size() == 1
		&& !_parent->path().is_root()
		&& !_arcs.front().must_mix();
}

} // namespace Server
} // namespace Ingen
