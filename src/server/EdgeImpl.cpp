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

#include "ingen/URIs.hpp"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "raul/log.hpp"

#include "Buffer.hpp"
#include "BufferFactory.hpp"
#include "EdgeImpl.hpp"
#include "Engine.hpp"
#include "InputPort.hpp"
#include "NodeImpl.hpp"
#include "OutputPort.hpp"
#include "PortImpl.hpp"

namespace Ingen {
namespace Server {

/** Constructor for a edge from a node's output port.
 *
 * This handles both polyphonic and monophonic nodes, transparently to the
 * user (InputPort).
 */
EdgeImpl::EdgeImpl(PortImpl* tail, PortImpl* head)
	: _tail(tail)
	, _head(head)
{
	assert(tail);
	assert(head);
	assert(tail != head);
	assert(tail->path() != head->path());
}

const Raul::Path&
EdgeImpl::tail_path() const
{
	return _tail->path();
}

const Raul::Path&
EdgeImpl::head_path() const
{
	return _head->path();
}

BufferRef
EdgeImpl::buffer(uint32_t voice) const
{
	assert(!must_mix());
	assert(_tail->poly() == 1 || _tail->poly() > voice);
	if (_tail->poly() == 1) {
		return _tail->buffer(0);
	} else {
		return _tail->buffer(voice);
	}
}

bool
EdgeImpl::must_mix() const
{
	return _tail->poly() > _head->poly();
}

bool
EdgeImpl::can_connect(const OutputPort* src, const InputPort* dst)
{
	const Ingen::URIs& uris = src->bufs().uris();
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

