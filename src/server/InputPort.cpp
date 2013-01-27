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
                     const Raul::Atom&   value,
                     size_t              buffer_size)
	: PortImpl(bufs, parent, symbol, index, poly, type, buffer_type, value, buffer_size)
	, _num_arcs(0)
{
	const Ingen::URIs& uris = bufs.uris();

	if (parent->graph_type() != Node::GraphType::GRAPH) {
		add_property(uris.rdf_type, uris.lv2_InputPort);
	}

	// Set default control range
	if (type == PortType::CONTROL || type == PortType::CV) {
		set_property(uris.lv2_minimum, bufs.forge().make(0.0f));
		set_property(uris.lv2_maximum, bufs.forge().make(1.0f));
	}
}

bool
InputPort::apply_poly(ProcessContext& context, Raul::Maid& maid, uint32_t poly)
{
	bool ret = PortImpl::apply_poly(context, maid, poly);
	if (!ret)
		poly = 1;

	assert(_buffers->size() >= poly);

	return true;
}

/** Set @a buffers to the buffers to be used for this port.
 * @return true iff buffers are locally owned by the port
 */
bool
InputPort::get_buffers(BufferFactory&          bufs,
                       Raul::Array<BufferRef>* buffers,
                       uint32_t                poly,
                       bool                    real_time) const
{
	const size_t num_arcs = real_time ? _arcs.size() : _num_arcs;

	if (is_a(PortType::AUDIO) && num_arcs == 0) {
		// Audio input with no arcs, use shared zero buffer
		for (uint32_t v = 0; v < poly; ++v) {
			buffers->at(v) = bufs.silent_buffer();
		}
		return false;

	} else if (num_arcs == 1) {
		if (real_time) {
			if (!_arcs.front().must_mix()) {
				// Single non-mixing connection, use buffers directly
				for (uint32_t v = 0; v < poly; ++v) {
					buffers->at(v) = _arcs.front().buffer(v);
				}
				return false;
			}
		}
	}

	// Otherwise, allocate local buffers
	for (uint32_t v = 0; v < poly; ++v) {
		buffers->at(v).reset();
		buffers->at(v) = bufs.get_buffer(buffer_type(), _buffer_size, real_time);
		buffers->at(v)->clear();
	}
	return true;
}

/** Add an arc.  Realtime safe.
 *
 * The buffer of this port will be set directly to the arc's buffer
 * if there is only one arc, since no copying/mixing needs to take place.
 *
 * Note that setup_buffers must be called after this before the change
 * will audibly take effect.
 */
void
InputPort::add_arc(ProcessContext& context, ArcImpl* c)
{
	_arcs.push_front(*c);
	if (_type != PortType::CV) {
		_broadcast = true;  // Broadcast value/activity of connected input
	}
}

/** Remove a arc.  Realtime safe.
 *
 * Note that setup_buffers must be called after this before the change
 * will audibly take effect.
 */
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

	if (_arcs.empty()) {
		_broadcast = false;  // Turn off broadcasting if no longer connected
	}

	return arc;
}

uint32_t
InputPort::max_tail_poly(Context& context) const
{
	return parent_block()->parent_graph()->internal_poly_process();
}

static void
get_sources(const Context& context,
            const ArcImpl& arc,
            uint32_t       voice,
            const Buffer** srcs,
            uint32_t       max_num_srcs,
            uint32_t&      num_srcs)
{
	if (arc.must_mix()) {
		// Mixing down voices: all tail voices => one head voice
		for (uint32_t v = 0; v < arc.tail()->poly(); ++v) {
			assert(num_srcs < max_num_srcs);
			srcs[num_srcs++] = arc.tail()->buffer(v).get();
		}
	} else {
		// Matching polyphony: each tail voice => corresponding head voice
		assert(arc.tail()->poly() == arc.head()->poly());
		assert(num_srcs < max_num_srcs);
		srcs[num_srcs++] = arc.tail()->buffer(voice).get();
	}
}

/** Prepare buffer for access, mixing if necessary.  Realtime safe.
 */
void
InputPort::pre_process(Context& context)
{
	// If value has been set (e.g. events pushed) by the user, don't smash it
	if (_set_by_user)
		return;

	if (_arcs.empty()) {
		for (uint32_t v = 0; v < _poly; ++v) {
			update_set_state(context, v);
		}
	} else if (direct_connect()) {
		for (uint32_t v = 0; v < _poly; ++v) {
			_buffers->at(v) = _arcs.front().buffer(v);
		}
	} else {
		const uint32_t src_poly     = max_tail_poly(context);
		const uint32_t max_num_srcs = _arcs.size() * src_poly;

		const Buffer* srcs[max_num_srcs];
		for (uint32_t v = 0; v < _poly; ++v) {
			// Get all the sources for this voice
			uint32_t num_srcs = 0;
			for (const auto& a : _arcs) {
				get_sources(context, a, v, srcs, max_num_srcs, num_srcs);
			}

			// Then mix them into out buffer for this voice
			mix(context, buffer(v).get(), srcs, num_srcs);
		}
	}

	if (_broadcast)
		broadcast_value(context, false);
}

void
InputPort::post_process(Context& context)
{
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

