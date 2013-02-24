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

#include "BlockImpl.hpp"
#include "Buffer.hpp"
#include "BufferFactory.hpp"
#include "Engine.hpp"
#include "Driver.hpp"
#include "PortImpl.hpp"
#include "PortType.hpp"
#include "ThreadManager.hpp"

using namespace std;

namespace Ingen {
namespace Server {

static const uint32_t monitor_rate = 10.0;  // Hz

/** The length of time between monitor updates in frames */
static inline uint32_t
monitor_period(const Engine& engine)
{
	return std::max(engine.driver()->block_length(),
	                engine.driver()->sample_rate() / monitor_rate);
}

PortImpl::PortImpl(BufferFactory&      bufs,
                   BlockImpl* const    block,
                   const Raul::Symbol& name,
                   uint32_t            index,
                   uint32_t            poly,
                   PortType            type,
                   LV2_URID            buffer_type,
                   const Atom&         value,
                   size_t              buffer_size)
	: NodeImpl(bufs.uris(), block, name)
	, _bufs(bufs)
	, _index(index)
	, _poly(poly)
	, _buffer_size(buffer_size)
	, _frames_since_monitor(0)
	, _monitor_value(0.0f)
	, _peak(0.0f)
	, _type(type)
	, _buffer_type(buffer_type)
	, _value(value)
	, _min(bufs.forge().make(0.0f))
	, _max(bufs.forge().make(1.0f))
	, _set_states(new Raul::Array<SetState>(static_cast<size_t>(poly)))
	, _prepared_set_states(NULL)
	, _buffers(new Raul::Array<BufferRef>(static_cast<size_t>(poly)))
	, _prepared_buffers(NULL)
	, _monitored(false)
	, _set_by_user(false)
	, _is_morph(false)
	, _is_auto_morph(false)
	, _is_logarithmic(false)
	, _is_sample_rate(false)
	, _is_toggled(false)
{
	assert(block != NULL);
	assert(_poly > 0);

	const Ingen::URIs& uris = bufs.uris();

	set_type(type, buffer_type);

	add_property(uris.rdf_type, bufs.forge().alloc_uri(type.uri()));
	set_property(uris.lv2_index, bufs.forge().make((int32_t)index));
	if ((type == PortType::CONTROL || type == PortType::CV) && value.is_valid()) {
		set_property(uris.ingen_value, value);
	}
	if (type == PortType::ATOM) {
		add_property(uris.atom_bufferType,
		             bufs.forge().make_urid(buffer_type));
		if (block->graph_type() == Ingen::Node::GraphType::GRAPH) {
			add_property(uris.atom_supports,
			             bufs.forge().make_urid(uris.midi_MidiEvent));
			add_property(uris.atom_supports,
			             bufs.forge().make_urid(uris.time_Position));
		}
	}
}

PortImpl::~PortImpl()
{
	delete _set_states;
	delete _buffers;
}

void
PortImpl::set_type(PortType port_type, LV2_URID buffer_type)
{
	_type        = port_type;
	_buffer_type = buffer_type;
	if (!_buffer_type) {
		switch (_type.id()) {
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

bool
PortImpl::supports(const Raul::URI& value_type) const
{
	return has_property(_bufs.uris().atom_supports,
	                    _bufs.forge().alloc_uri(value_type));
}

void
PortImpl::activate(BufferFactory& bufs)
{
	setup_buffers(bufs, _poly, false);
	connect_buffers();
	clear_buffers();

	/* Set the time since the last monitor update to a random value within the
	   monitor period, to spread the load out over time.  Otherwise, every
	   port would try to send an update at exactly the same time, every time.
	*/
	const double   srate  = bufs.engine().driver()->sample_rate();
	const uint32_t period = srate / monitor_rate;
	_frames_since_monitor = bufs.engine().frand() * period;
	_monitor_value        = 0.0f;
	_peak                 = 0.0f;
}

void
PortImpl::deactivate()
{
	if (is_output()) {
		for (uint32_t v = 0; v < _poly; ++v) {
			if (_buffers->at(v)) {
				_buffers->at(v)->clear();
			}
		}
	}
	_monitor_value = 0.0f;
	_peak          = 0.0f;
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
PortImpl::set_control_value(const Context& context,
                            FrameTime      time,
                            Sample         value)
{
	for (uint32_t v = 0; v < _poly; ++v) {
		set_voice_value(context, v, time, value);
	}
}

void
PortImpl::set_voice_value(const Context& context,
                          uint32_t       voice,
                          FrameTime      time,
                          Sample         value)
{
	switch (_type.id()) {
	case PortType::CONTROL:
		buffer(voice)->samples()[0] = value;
		_set_states->at(voice).state = SetState::State::SET;
		break;
	case PortType::AUDIO:
	case PortType::CV: {
		// Time may be at end so internal blocks can set triggers
		assert(time >= context.start());
		assert(time <= context.start() + context.nframes());

		const FrameTime offset = time - context.start();
		if (offset < context.nframes()) {
			buffer(voice)->set_block(value, offset, context.nframes());
		}
		/* else, this is a set at context.nframes(), used to reset a CV port's
		   value for the next block, particularly for triggers on the last
		   frame of a block (set nframes-1 to 1, then nframes to 0). */

		SetState& state = _set_states->at(voice);
		state.state = (offset == 0)
			? SetState::State::SET
			: SetState::State::HALF_SET_CYCLE_1;
		state.time  = time;
		state.value = value;
	}
	default:
		break;
	}
}

void
PortImpl::update_set_state(Context& context, uint32_t voice)
{
	SetState& state = _set_states->at(voice);
	switch (state.state) {
	case SetState::State::SET:
		break;
	case SetState::State::HALF_SET_CYCLE_1:
		state.state = SetState::State::HALF_SET_CYCLE_2;
		break;
	case SetState::State::HALF_SET_CYCLE_2:
		buffer(voice)->set_block(state.value, 0, context.nframes());
		state.state = SetState::State::SET;
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

	get_buffers(bufs,
	            _prepared_buffers,
	            _prepared_buffers->size(),
	            false);

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
	maid.dispose(set_buffers(context, _prepared_buffers));
	assert(_buffers == _prepared_buffers);
	_prepared_buffers = NULL;

	maid.dispose(_set_states);
	_set_states          = _prepared_set_states;
	_prepared_set_states = NULL;

	if (is_a(PortType::CONTROL) || is_a(PortType::CV)) {
		set_control_value(context, context.start(), _value.get<float>());
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
		PortImpl::parent_block()->set_port_buffer(v, _index, buffer(v));
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
	switch (_type.id()) {
	case PortType::AUDIO:
	case PortType::CONTROL:
	case PortType::CV:
		for (uint32_t v = 0; v < _poly; ++v) {
			Buffer* buf = buffer(v).get();
			buf->set_block(_value.get<float>(), 0, buf->nframes());
			SetState& state = _set_states->at(v);
			state.state = SetState::State::SET;
			state.value = _value.get<float>();
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
PortImpl::monitor(Context& context, bool send_now)
{
	if (!context.must_notify(this)) {
		return;
	}

	Forge& forge = context.engine().world()->forge();
	URIs&  uris  = context.engine().world()->uris();

	LV2_URID key = 0;
	float    val = 0.0f;
	switch (_type.id()) {
	case PortType::UNKNOWN:
		break;
	case PortType::AUDIO:
		key = uris.ingen_activity;
		val = _peak = std::max(_peak, buffer(0)->peak(context));
		break;
	case PortType::CONTROL:
	case PortType::CV:
		key = uris.ingen_value;
		val = buffer(0)->value_at(0);
		break;
	case PortType::ATOM:
		if (_buffer_type == _bufs.uris().atom_Sequence) {
			LV2_Atom_Sequence* seq = (LV2_Atom_Sequence*)buffer(0)->atom();
			if (_monitored) {
				// Monitoring explictly enabled, send everything
				LV2_ATOM_SEQUENCE_FOREACH(seq, ev) {
					context.notify(uris.ingen_activity,
					               context.start() + ev->time.frames,
					               this,
					               ev->body.size,
					               ev->body.type,
					               LV2_ATOM_BODY(&ev->body));
				}
			} else if (seq->atom.size > sizeof(LV2_Atom_Sequence_Body)) {
				// Just sending for blinkenlights, send one
				const int32_t one = 1;
				context.notify(uris.ingen_activity,
				               context.start(),
				               this,
				               sizeof(int32_t),
				               (LV2_URID)uris.atom_Bool,
				               &one);
			}
		}
	}

	const uint32_t period       = monitor_period(context.engine());
	const bool     time_to_send = send_now || _frames_since_monitor >= period;
	if (time_to_send && key && val != _monitor_value) {
		if (context.notify(key, context.start(), this,
		                   sizeof(float), forge.Float, &val)) {
			/* Update frames since last update to conceptually zero, but keep
			   the remainder to preserve load balancing. */
			_frames_since_monitor = _frames_since_monitor % period;
			_peak                 = 0.0f;
			_monitor_value        = val;
		}
		// Otherwise failure, leave old value and try again next time
	}

	_frames_since_monitor += context.nframes();
}

} // namespace Server
} // namespace Ingen
