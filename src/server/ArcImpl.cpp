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

#include "ArcImpl.hpp"

#include "Buffer.hpp"
#include "BufferFactory.hpp"
#include "InputPort.hpp"
#include "PortImpl.hpp"
#include "PortType.hpp"

#include "ingen/URIs.hpp"
#include "raul/Path.hpp"

#include <algorithm>
#include <cassert>

namespace ingen {
namespace server {

/** Constructor for an arc from a block's output port.
 *
 * This handles both polyphonic and monophonic blocks, transparently to the
 * user (InputPort).
 */
ArcImpl::ArcImpl(PortImpl* tail, PortImpl* head)
	: _tail(tail)
	, _head(head)
{
	assert(tail != head);
	assert(tail->path() != head->path());
}

ArcImpl::~ArcImpl()
{
	if (is_linked()) {
		auto* iport = dynamic_cast<InputPort*>(_head);
		if (iport) {
			iport->remove_arc(*this);
		}
	}
}

const raul::Path&
ArcImpl::tail_path() const
{
	return _tail->path();
}

const raul::Path&
ArcImpl::head_path() const
{
	return _head->path();
}

BufferRef
ArcImpl::buffer(const RunContext&, uint32_t voice) const
{
	return _tail->buffer(std::min(voice, _tail->poly() - 1));
}

bool
ArcImpl::must_mix() const
{
	return (_tail->poly() > _head->poly() ||
	        (_tail->buffer(0)->is_sequence() != _head->buffer(0)->is_sequence()));
}

bool
ArcImpl::can_connect(const PortImpl* src, const InputPort* dst)
{
	const ingen::URIs& uris = src->bufs().uris();
	return (
		// (Audio | Control | CV) => (Audio | Control | CV)
		(   (src->is_a(PortType::ID::CONTROL) ||
		     src->is_a(PortType::ID::AUDIO) ||
		     src->is_a(PortType::ID::CV))
		    && (dst->is_a(PortType::ID::CONTROL)
		        || dst->is_a(PortType::ID::AUDIO)
		        || dst->is_a(PortType::ID::CV)))

		// Equal types
		|| (src->type() == dst->type() &&
		    src->buffer_type() == dst->buffer_type())

		// Control => atom:Float Value
		|| (src->is_a(PortType::ID::CONTROL) && dst->supports(uris.atom_Float))

		// Audio => atom:Sound Value
		|| (src->is_a(PortType::ID::AUDIO) && dst->supports(uris.atom_Sound))

		// atom:Float Value => Control
		|| (src->supports(uris.atom_Float) && dst->is_a(PortType::ID::CONTROL))

		// atom:Float Value => CV
		|| (src->supports(uris.atom_Float) && dst->is_a(PortType::ID::CV))

		// atom:Sound Value => Audio
		|| (src->supports(uris.atom_Sound) && dst->is_a(PortType::ID::AUDIO)));
}

} // namespace server
} // namespace ingen
