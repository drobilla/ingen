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
#include "lv2/lv2plug.in/ns/ext/worker/worker.h"
#include "raul/Array.hpp"
#include "raul/Maid.hpp"

#include "AudioBuffer.hpp"
#include "BufferFactory.hpp"
#include "Engine.hpp"
#include "NodeImpl.hpp"
#include "Notification.hpp"
#include "PortImpl.hpp"
#include "PortType.hpp"
#include "ThreadManager.hpp"

using namespace std;

namespace Ingen {
namespace Server {

PortImpl::PortImpl(BufferFactory&      bufs,
                   NodeImpl* const     node,
                   const Raul::Symbol& name,
                   uint32_t            index,
                   uint32_t            poly,
                   PortType            type,
                   LV2_URID            buffer_type,
                   const Raul::Atom&   value,
                   size_t              buffer_size)
	: GraphObjectImpl(bufs.uris(), node, name)
	, _bufs(bufs)
	, _index(index)
	, _poly(poly)
	, _buffer_size(buffer_size)
	, _type(type)
	, _buffer_type(buffer_type)
	, _value(value)
	, _min(bufs.forge().make(0.0f))
	, _max(bufs.forge().make(1.0f))
	, _last_broadcasted_value(value)
	, _buffers(new Raul::Array<BufferRef>(static_cast<size_t>(poly)))
	, _prepared_buffers(NULL)
	, _broadcast(false)
	, _set_by_user(false)
{
	assert(node != NULL);
	assert(_poly > 0);

	const Ingen::Shared::URIs& uris = bufs.uris();

	if (_buffer_size == 0) {
		_buffer_size = bufs.default_buffer_size(buffer_type);
	}

	if (_buffer_type == 0) {
		switch (_type.symbol()) {
		case PortType::CONTROL:
			_buffer_type = uris.atom_Float;
			break;
		case PortType::AUDIO:
		case PortType::CV:
			_buffer_type = uris.atom_Sound;
			break;
		default:
			break;
		}
	}

	add_property(uris.rdf_type,  bufs.forge().alloc_uri(type.uri().str()));
	set_property(uris.lv2_index, bufs.forge().make((int32_t)index));
}

PortImpl::~PortImpl()
{
	delete _buffers;
}

bool
PortImpl::supports(const Raul::URI& value_type) const
{
	return has_property(_bufs.uris().atom_supports,
	                    _bufs.forge().alloc_uri(value_type.str()));
}

Raul::Array<BufferRef>*
PortImpl::set_buffers(Raul::Array<BufferRef>* buffers)
{
	ThreadManager::assert_thread(THREAD_PROCESS);

	Raul::Array<BufferRef>* ret = NULL;
	if (buffers != _buffers) {
		ret = _buffers;
		_buffers = buffers;
	}

	connect_buffers();

	return ret;
}

bool
PortImpl::prepare_poly(BufferFactory& bufs, uint32_t poly)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);
	if (_type != PortType::CONTROL &&
	    _type != PortType::CV &&
	    _type != PortType::AUDIO) {
		return false;
	}

	if (_poly == poly) {
		return true;
	}

	if (_prepared_buffers && _prepared_buffers->size() != poly) {
		delete _prepared_buffers;
		_prepared_buffers = NULL;
	}

	if (!_prepared_buffers)
		_prepared_buffers = new Raul::Array<BufferRef>(poly, *_buffers, NULL);

	return true;
}

void
PortImpl::prepare_poly_buffers(BufferFactory& bufs)
{
	if (_prepared_buffers)
		get_buffers(bufs, _prepared_buffers, _prepared_buffers->size());
}

bool
PortImpl::apply_poly(Raul::Maid& maid, uint32_t poly)
{
	ThreadManager::assert_thread(THREAD_PROCESS);
	if (_type != PortType::CONTROL &&
	    _type != PortType::CV &&
	    _type != PortType::AUDIO) {
		return false;
	}

	if (!_prepared_buffers) {
		return true;
	}

	assert(poly == _prepared_buffers->size());

	_poly = poly;

	// Apply a new set of buffers from a preceding call to prepare_poly
	maid.push(set_buffers(_prepared_buffers));
	assert(_buffers == _prepared_buffers);
	_prepared_buffers = NULL;

	if (is_a(PortType::CONTROL) || is_a(PortType::CV))
		for (uint32_t v = 0; v < _poly; ++v)
			if (_buffers->at(v))
				boost::static_pointer_cast<AudioBuffer>(_buffers->at(v))->set_value(
						_value.get_float(), 0, 0);

	assert(_buffers->size() >= poly);
	assert(this->poly() == poly);
	assert(!_prepared_buffers);

	return true;
}

void
PortImpl::set_buffer_size(Context& context, BufferFactory& bufs, size_t size)
{
	_buffer_size = size;

	for (uint32_t v = 0; v < _poly; ++v)
		_buffers->at(v)->resize(size);

	connect_buffers();
}

void
PortImpl::connect_buffers(SampleCount offset)
{
	for (uint32_t v = 0; v < _poly; ++v)
		PortImpl::parent_node()->set_port_buffer(v, _index, buffer(v), offset);
}

void
PortImpl::recycle_buffers()
{
	for (uint32_t v = 0; v < _poly; ++v)
		_buffers->at(v) = NULL;
}

void
PortImpl::clear_buffers()
{
	for (uint32_t v = 0; v < _poly; ++v)
		buffer(v)->clear();
}

void
PortImpl::broadcast_value(Context& context, bool force)
{
	Shared::Forge&     forge = context.engine().world()->forge();
	Notification::Type ntype = Notification::PORT_VALUE;
	Raul::Atom         val;
	switch (_type.symbol()) {
	case PortType::UNKNOWN:
		break;
	case PortType::AUDIO:
		val   = forge.make(((AudioBuffer*)buffer(0).get())->peak(context));
		ntype = Notification::PORT_ACTIVITY;
		break;
	case PortType::CONTROL:
	case PortType::CV:
		val = forge.make(((AudioBuffer*)buffer(0).get())->value_at(0));
		break;
	case PortType::ATOM:
		if (_buffer_type == _bufs.uris().atom_Sequence) {
			LV2_Atom_Sequence* seq = (LV2_Atom_Sequence*)buffer(0)->atom();
			if (seq->atom.size > sizeof(LV2_Atom_Sequence_Body)) {
				const Notification note = Notification::make(
					Notification::PORT_ACTIVITY, context.start(), this, forge.make(true));
				context.event_sink().write(sizeof(note), &note);
			}
		}
		break;
	}

	if (val.is_valid() && (force || val != _last_broadcasted_value)) {
		_last_broadcasted_value = val;
		const Notification note = Notification::make(
			ntype, context.start(), this, val);
		context.event_sink().write(sizeof(note), &note);
	}
}

} // namespace Server
} // namespace Ingen
