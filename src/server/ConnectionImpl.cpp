/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
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

#include <algorithm>
#include <boost/intrusive_ptr.hpp>

#include "ingen/shared/LV2URIMap.hpp"
#include "ingen/shared/URIs.hpp"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "raul/Maid.hpp"
#include "raul/log.hpp"

#include "AudioBuffer.hpp"
#include "BufferFactory.hpp"
#include "ConnectionImpl.hpp"
#include "Engine.hpp"
#include "InputPort.hpp"
#include "MessageContext.hpp"
#include "NodeImpl.hpp"
#include "OutputPort.hpp"
#include "PortImpl.hpp"
#include "ProcessContext.hpp"
#include "mix.hpp"
#include "util.hpp"

namespace Ingen {
namespace Server {

/** Constructor for a connection from a node's output port.
 *
 * This handles both polyphonic and monophonic nodes, transparently to the
 * user (InputPort).
 */
ConnectionImpl::ConnectionImpl(PortImpl* src_port, PortImpl* dst_port)
	: _src_port(src_port)
	, _dst_port(dst_port)
	, _queue(NULL)
{
	assert(src_port);
	assert(dst_port);
	assert(src_port != dst_port);
	assert(src_port->path() != dst_port->path());

	if (must_queue())
		_queue = new Raul::RingBuffer(src_port->buffer_size() * 2);
}

void
ConnectionImpl::dump() const
{
	debug << _src_port->path() << " -> " << _dst_port->path()
		<< (must_mix()   ? " (MIX) " : " (DIRECT) ")
		<< (must_queue() ? " (QUEUE)" : " (NOQUEUE) ")
		<< "POLY: " << _src_port->poly() << " => " << _dst_port->poly() << endl;
}

const Raul::Path&
ConnectionImpl::src_port_path() const
{
	return _src_port->path();
}

const Raul::Path&
ConnectionImpl::dst_port_path() const
{
	return _dst_port->path();
}

void
ConnectionImpl::get_sources(Context& context, uint32_t voice,
		boost::intrusive_ptr<Buffer>* srcs, uint32_t max_num_srcs, uint32_t& num_srcs)
{
	if (must_queue() && _queue->read_space() > 0) {
		LV2_Atom obj;
		_queue->peek(sizeof(LV2_Atom), &obj);
		boost::intrusive_ptr<Buffer> buf = context.engine().buffer_factory()->get(
				dst_port()->buffer_type(), sizeof(LV2_Atom) + obj.size);
		void* data = buf->port_data(PortType::MESSAGE, context.offset());
		_queue->read(sizeof(LV2_Atom) + obj.size, (LV2_Atom*)data);
		srcs[num_srcs++] = buf;
	} else if (must_mix()) {
		// Mixing down voices: every src voice mixed into every dst voice
		for (uint32_t v = 0; v < _src_port->poly(); ++v) {
			assert(num_srcs < max_num_srcs);
			srcs[num_srcs++] = _src_port->buffer(v).get();
		}
	} else {
		// Matching polyphony: each src voice mixed into corresponding dst voice
		assert(_src_port->poly() == _dst_port->poly());
		assert(num_srcs < max_num_srcs);
		srcs[num_srcs++] = _src_port->buffer(voice).get();
	}
}

void
ConnectionImpl::queue(Context& context)
{
	if (!must_queue())
		return;

	const Ingen::Shared::URIs& uris = _src_port->bufs().uris();

	boost::intrusive_ptr<Buffer> src_buf = _src_port->buffer(0);
	if (src_buf->atom()->type != uris.atom_Sequence) {
		error << "Queued connection but source is not a Sequence" << endl;
		return;
	}

	LV2_Atom_Sequence* seq = (LV2_Atom_Sequence*)src_buf->atom();
	LV2_SEQUENCE_FOREACH(seq, i) {
		LV2_Atom_Event* const ev = lv2_sequence_iter_get(i);
		_queue->write(sizeof(LV2_Atom) + ev->body.size, &ev->body);
		context.engine().message_context()->run(
			_dst_port->parent_node(), context.start() + ev->time.frames);

	}
}

BufferFactory::Ref
ConnectionImpl::buffer(uint32_t voice) const
{
	assert(!must_mix());
	assert(!must_queue());
	assert(_src_port->poly() == 1 || _src_port->poly() > voice);
	if (_src_port->poly() == 1) {
		return _src_port->buffer(0);
	} else {
		return _src_port->buffer(voice);
	}
}

bool
ConnectionImpl::must_mix() const
{
	return _src_port->poly() > _dst_port->poly();
}

bool
ConnectionImpl::must_queue() const
{
	return _src_port->parent_node()->context()
		!= _dst_port->parent_node()->context();
}

bool
ConnectionImpl::can_connect(const OutputPort* src, const InputPort* dst)
{
	const Ingen::Shared::URIs& uris = src->bufs().uris();
	return (
			// (Audio | Control | CV) => (Audio | Control | CV)
			(   (src->is_a(PortType::CONTROL) ||
			     src->is_a(PortType::AUDIO) ||
			     src->is_a(PortType::CV))
			 && (dst->is_a(PortType::CONTROL)
			     || dst->is_a(PortType::AUDIO)
			     || dst->is_a(PortType::CV)))

			// Equal types
			|| (src->type() == dst->type() &&
			    src->buffer_type() == dst->buffer_type())

			// Control => atom:Float Value
			|| (src->is_a(PortType::CONTROL) && dst->supports(uris.atom_Float))

			// Audio => atom:Sound Value
			|| (src->is_a(PortType::AUDIO) && dst->supports(uris.atom_Sound))

			// atom:Float Value => Control
			|| (src->supports(uris.atom_Float) && dst->is_a(PortType::CONTROL))

			// atom:Sound Value => Audio
			|| (src->supports(uris.atom_Sound) && dst->is_a(PortType::AUDIO)));
}

} // namespace Server
} // namespace Ingen

