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
#include "raul/Array.hpp"
#include "raul/Maid.hpp"

#include "Buffer.hpp"
#include "BufferFactory.hpp"
#include "Engine.hpp"
#include "NodeImpl.hpp"
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
	, _set_states(new Raul::Array<SetState>(static_cast<size_t>(poly)))
	, _prepared_set_states(NULL)
	, _buffers(new Raul::Array<BufferRef>(static_cast<size_t>(poly)))
	, _prepared_buffers(NULL)
	, _broadcast(false)
	, _set_by_user(false)
	, _is_morph(false)
	, _is_auto_morph(false)
	, _is_logarithmic(false)
	, _is_sample_rate(false)
	, _is_toggled(false)
{
	assert(node != NULL);
	assert(_poly > 0);

	const Ingen::URIs& uris = bufs.uris();

	set_type(type, buffer_type);

	add_property(uris.atom_bufferType, bufs.forge().make_urid(buffer_type));
	add_property(uris.rdf_type, bufs.forge().alloc_uri(type.uri().str()));
	set_property(uris.lv2_index, bufs.forge().make((int32_t)index));
}

void
PortImpl::set_type(PortType port_type, LV2_URID buffer_type)
{
	_type        = port_type;
	_buffer_type = buffer_type;
	if (!_buffer_type) {
		switch (_type.symbol()) {
		case PortType::CONTROL:
			_buffer_type = _bufs.uris().atom_Float;
			break;
		case PortType::AUDIO:
		case PortType::CV:
			_buffer_type = _bufs.uris().atom_Sound;
			break;
		default:
			break;
		}
	}
	_buffer_size = _bufs.default_size(_buffer_type);
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
PortImpl::set_buffers(ProcessContext& context, Raul::Array<BufferRef>* buffers)
{
	Raul::Array<BufferRef>* ret = NULL;
	if (buffers != _buffers) {
		ret = _buffers;
		_buffers = buffers;
	}

	connect_buffers();

	return ret;
}

void
PortImpl::cache_properties()
{
	_is_logarithmic = has_property(_bufs.uris().lv2_portProperty,
	                               _bufs.uris().pprops_logarithmic);
	_is_sample_rate = has_property(_bufs.uris().lv2_portProperty,
	                               _bufs.uris().lv2_sampleRate);
	_is_toggled = has_property(_bufs.uris().lv2_portProperty,
	                           _bufs.uris().lv2_toggled);
}

void
PortImpl::set_control_value(Context& context, FrameTime time, Sample value)
{
	for (uint32_t v = 0; v < poly(); ++v) {
		set_voice_value(context, v, time, value);
	}
}

void
PortImpl::set_voice_value(Context& context, uint32_t voice, FrameTime time, Sample value)
{
	// Time may be at end so internal nodes can set single sample triggers
	assert(time >= context.start());
	assert(time <= context.start() + context.nframes());

	if (_type == PortType::CONTROL) {
		time = context.start();
	}

	FrameTime offset = time - context.start();
	FrameTime end    = (_type == PortType::CONTROL) ? 1 : context.nframes();
	if (offset < context.nframes()) {
		buffer(voice)->set_block(value, offset, end);
	} // else trigger at very end of block, and set to 0 at start of next block

	SetState& state = _set_states->at(voice);
	state.state = (offset == 0) ? SetState::SET : SetState::HALF_SET_CYCLE_1;
	state.time  = time;
	state.value = value;
}

void
PortImpl::update_set_state(Context& context, uint32_t voice)
{
	SetState& state = _set_states->at(voice);
	switch (state.state) {
	case SetState::HALF_SET_CYCLE_1:
		if (context.start() > state.time) {
			state.state = SetState::HALF_SET_CYCLE_2;
		}
		break;
	case SetState::HALF_SET_CYCLE_2: {
		buffer(voice)->set_block(state.value, 0, context.nframes());
		state.state = SetState::SET;
		break;
	}
	default:
		break;
	}
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
	
	if (_prepared_set_states && _prepared_set_states->size() != poly) {
		delete _prepared_set_states;
		_prepared_set_states = NULL;
	}

	if (!_prepared_buffers)
		_prepared_buffers = new Raul::Array<BufferRef>(poly, *_buffers, NULL);

	if (!_prepared_set_states)
		_prepared_set_states = new Raul::Array<SetState>(poly, *_set_states, SetState());

	get_buffers(bufs.engine().message_context(),
	            bufs,
	            _prepared_buffers,
	            _prepared_buffers->size());

	return true;
}

bool
PortImpl::apply_poly(ProcessContext& context, Raul::Maid& maid, uint32_t poly)
{
	if (_type != PortType::CONTROL &&
	    _type != PortType::CV &&
	    _type != PortType::AUDIO) {
		return false;
	}

	if (!_prepared_buffers) {
		return true;
	}

	assert(poly == _prepared_buffers->size());
	assert(poly == _prepared_set_states->size());

	_poly = poly;

	// Apply a new set of buffers from a preceding call to prepare_poly
	maid.push(set_buffers(context, _prepared_buffers));
	assert(_buffers == _prepared_buffers);
	_prepared_buffers = NULL;

	maid.push(_set_states);
	_set_states          = _prepared_set_states;
	_prepared_set_states = NULL;

	if (is_a(PortType::CONTROL) || is_a(PortType::CV)) {
		set_control_value(context, context.start(), _value.get_float());
	}

	assert(_buffers->size() >= poly);
	assert(_set_states->size() >= poly);
	assert(this->poly() == poly);
	assert(!_prepared_buffers);
	assert(!_prepared_set_states);

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
PortImpl::connect_buffers()
{
	for (uint32_t v = 0; v < _poly; ++v)
		PortImpl::parent_node()->set_port_buffer(v, _index, buffer(v));
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
	switch (_type.symbol()) {
	case PortType::AUDIO:
	case PortType::CONTROL:
	case PortType::CV:
		for (uint32_t v = 0; v < _poly; ++v) {
			Buffer* buf = buffer(v).get();
			buf->set_block(_value.get_float(), 0, buf->nframes());
			SetState& state = _set_states->at(v);
			state.state = SetState::SET;
			state.value = _value.get_float();
			state.time  = 0;
		}
		break;
	default:
		for (uint32_t v = 0; v < _poly; ++v) {
			buffer(v)->clear();
		}
	}
}

void
PortImpl::broadcast_value(Context& context, bool force)
{
	Forge& forge = context.engine().world()->forge();
	URIs&  uris  = context.engine().world()->uris();
	LV2_URID       key   = 0;
	Raul::Atom     val;
	switch (_type.symbol()) {
	case PortType::UNKNOWN:
		break;
	case PortType::AUDIO:
		key = uris.ingen_activity;
		val = forge.make(buffer(0)->peak(context));
		break;
	case PortType::CONTROL:
	case PortType::CV:
		key = uris.ingen_value;
		val = forge.make(buffer(0)->value_at(0));
		break;
	case PortType::ATOM:
		if (_buffer_type == _bufs.uris().atom_Sequence) {
			LV2_Atom_Sequence* seq = (LV2_Atom_Sequence*)buffer(0)->atom();
			// TODO: Filter events, or only send one activity for blinkenlights
			LV2_ATOM_SEQUENCE_FOREACH(seq, ev) {
				context.notify(uris.ingen_activity,
				               context.start() + ev->time.frames,
				               this,
				               ev->body.size,
				               ev->body.type,
				               LV2_ATOM_BODY(&ev->body));
			}
		}
		break;
	}

	if (val.is_valid() && (force || val != _last_broadcasted_value)) {
		_last_broadcasted_value = val;
		context.notify(key, context.start(), this,
		               val.size(), val.type(), val.get_body());
	}
}

} // namespace Server
} // namespace Ingen
