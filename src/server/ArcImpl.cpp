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

#include "ingen/URIs.hpp"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"

#include "ArcImpl.hpp"
#include "BlockImpl.hpp"
#include "Buffer.hpp"
#include "BufferFactory.hpp"
#include "Engine.hpp"
#include "InputPort.hpp"
#include "OutputPort.hpp"
#include "PortImpl.hpp"

namespace Ingen {
namespace Server {

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
		InputPort* iport = dynamic_cast<InputPort*>(_head);
		if (iport) {
			iport->remove_arc(*this);
		}
	}
}

const Raul::Path&
ArcImpl::tail_path() const
{
	return _tail->path();
}

const Raul::Path&
ArcImpl::head_path() const
{
	return _head->path();
}

BufferRef
ArcImpl::buffer(uint32_t voice, SampleCount offset) const
{
	assert(_tail->poly() == 1 || _tail->poly() > voice);
	if (_tail->poly() == 1) {
		voice = 0;
	}

	if (_tail->buffer(0)->is_sequence()) {
		if (_head->type() == PortType::CONTROL) {
			_tail->update_values(offset, voice);  // Update value buffer
			return _tail->value_buffer(voice);  // Return value buffer
		} else if (_head->type() == PortType::CV) {
			// Return full tail buffer below
		}
	}

	return _tail->buffer(voice);
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
	const Ingen::URIs& uris = src->bufs().uris();
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

} // namespace Server
} // namespace Ingen
