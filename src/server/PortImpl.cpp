/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

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

static const uint32_t monitor_rate = 25.0;  // Hz

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
	, _voices(new Raul::Array<Voice>(static_cast<size_t>(poly)))
	, _prepared_voices(NULL)
	, _monitored(false)
	, _force_monitor_update(false)
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

	set_property(uris.lv2_index, bufs.forge().make((int32_t)index));
	if (has_value()) {
		set_property(uris.ingen_value, value);
	}
	if (type == PortType::ATOM) {
		set_property(uris.atom_bufferType,
		             bufs.forge().make_urid(buffer_type));
	}
}

PortImpl::~PortImpl()
{
	delete _voices;
}

void
PortImpl::set_type(PortType port_type, LV2_URID buffer_type)
{
	const Ingen::URIs& uris  = _bufs.uris();
	Ingen::World*      world = _bufs.engine().world();

	// Update type properties so clients are aware of current type
	remove_property(uris.rdf_type, uris.lv2_AudioPort);
	remove_property(uris.rdf_type, uris.lv2_CVPort);
	remove_property(uris.rdf_type, uris.lv2_ControlPort);
	remove_property(uris.rdf_type, uris.atom_AtomPort);
	add_property(uris.rdf_type,
	             world->forge().alloc_uri(port_type.uri().c_str()));

	// Update audio thread types
	_type        = port_type;
	_buffer_type = buffer_type;
	if (!_buffer_type) {
		switch (_type.id()) {
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
	_buffer_size = std::max(_buffer_size, _bufs.default_size(_buffer_type));
}

bool
PortImpl::has_value() const
{
	return (_type == PortType::CONTROL ||
	        _type == PortType::CV ||
	        (_type == PortType::ATOM &&
	         _value.type() == _bufs.uris().atom_Float));
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
			if (_voices->at(v).buffer) {
				_voices->at(v).buffer->clear();
			}
		}
	}
	_monitor_value = 0.0f;
	_peak          = 0.0f;
}

Raul::Array<PortImpl::Voice>*
PortImpl::set_voices(ProcessContext& context, Raul::Array<Voice>* voices)
{
	Raul::Array<Voice>* ret = NULL;
	if (voices != _voices) {
		ret = _voices;
		_voices = voices;
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
		_voices->at(voice).set_state.set(context, context.start(), value);
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

		_voices->at(voice).set_state.set(context, time, value);
	} break;
	case PortType::ATOM:
		if (buffer(voice)->is_sequence()) {
			const FrameTime offset = time - context.start();
			// Same deal as above
			if (offset < context.nframes()) {
				buffer(voice)->append_event(offset,
				                            sizeof(value),
				                            _bufs.uris().atom_Float,
				                            (const uint8_t*)&value);
			}
			_voices->at(voice).set_state.set(context, time, value);
		} else {
			fprintf(stderr,
			        "error: %s set non-sequence atom port value (buffer type %u)\n",
			        path().c_str(), buffer(voice)->type());
		}
	default:
		break;
	}
}

void
PortImpl::update_set_state(Context& context, uint32_t voice)
{
	SetState& state = _voices->at(voice).set_state;
	switch (state.state) {
	case SetState::State::SET:
		if (state.time < context.start() &&
		    buffer(voice)->is_sequence() &&
		    buffer(voice)->value_type() == _bufs.uris().atom_Float &&
		    !_parent->path().is_root()) {
			buffer(voice)->clear();
			state.time = context.start();
		}
		break;
	case SetState::State::HALF_SET_CYCLE_1:
		state.state = SetState::State::HALF_SET_CYCLE_2;
		break;
	case SetState::State::HALF_SET_CYCLE_2:
		if (buffer(voice)->is_sequence()) {
			buffer(voice)->clear();
			buffer(voice)->append_event(
				0, sizeof(float), _bufs.uris().atom_Float,
				(const uint8_t*)&state.value);
		} else {
			buffer(voice)->set_block(state.value, 0, context.nframes());
		}
		state.state = SetState::State::SET;
		break;
	}
}

bool
PortImpl::prepare_poly(BufferFactory& bufs, uint32_t poly)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);
	if (_parent->path().is_root() ||
	    (_type == PortType::ATOM && !_value.is_valid())) {
		return false;
	}

	if (_poly == poly) {
		return true;
	}

	if (_prepared_voices && _prepared_voices->size() != poly) {
		delete _prepared_voices;
		_prepared_voices = NULL;
	}

	if (!_prepared_voices)
		_prepared_voices = new Raul::Array<Voice>(poly, *_voices, Voice());

	get_buffers(bufs,
	            _prepared_voices,
	            _prepared_voices->size(),
	            false);

	return true;
}

bool
PortImpl::apply_poly(ProcessContext& context, Raul::Maid& maid, uint32_t poly)
{
	if (_parent->path().is_root() ||
	    (_type == PortType::ATOM && !_value.is_valid())) {
		return false;
	}

	if (!_prepared_voices) {
		return true;
	}

	assert(poly == _prepared_voices->size());

	_poly = poly;

	// Apply a new set of voices from a preceding call to prepare_poly
	maid.dispose(set_voices(context, _prepared_voices));
	assert(_voices == _prepared_voices);
	_prepared_voices = NULL;

	if (is_a(PortType::CONTROL) || is_a(PortType::CV)) {
		set_control_value(context, context.start(), _value.get<float>());
	}

	assert(_voices->size() >= poly);
	assert(this->poly() == poly);
	assert(!_prepared_voices);

	return true;
}

void
PortImpl::set_buffer_size(Context& context, BufferFactory& bufs, size_t size)
{
	_buffer_size = size;

	for (uint32_t v = 0; v < _poly; ++v)
		_voices->at(v).buffer->resize(size);

	connect_buffers();
}

void
PortImpl::connect_buffers(SampleCount offset)
{
	for (uint32_t v = 0; v < _poly; ++v)
		PortImpl::parent_block()->set_port_buffer(v, _index, buffer(v), offset);
}

void
PortImpl::recycle_buffers()
{
	for (uint32_t v = 0; v < _poly; ++v)
		_voices->at(v).buffer = NULL;
}

void
PortImpl::clear_buffers()
{
	switch (_type.id()) {
	case PortType::CONTROL:
	case PortType::CV:
		for (uint32_t v = 0; v < _poly; ++v) {
			Buffer* buf = buffer(v).get();
			buf->set_block(_value.get<float>(), 0, buf->nframes());
			SetState& state = _voices->at(v).set_state;
			state.state = SetState::State::SET;
			state.value = _value.get<float>();
			state.time  = 0;
		}
		break;
	case PortType::AUDIO:
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

	const uint32_t period = monitor_period(context.engine());
	_frames_since_monitor += context.nframes();

	const bool time_to_send = send_now || _frames_since_monitor >= period;
	const bool is_sequence  = (_type.id() == PortType::ATOM &&
	                           _buffer_type == _bufs.uris().atom_Sequence);
	if (!time_to_send && (!is_sequence || _monitored || buffer(0)->value())) {
		return;
	}

	Forge&   forge = context.engine().world()->forge();
	URIs&    uris  = context.engine().world()->uris();
	LV2_URID key   = 0;
	float    val   = 0.0f;
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
			const LV2_Atom* atom = buffer(0)->atom();
			if (buffer(0)->value() && !_monitored) {
				// Value sequence not fully monitored, monitor as control
				key = uris.ingen_value;
				val = ((LV2_Atom_Float*)buffer(0)->value())->body;
			} else if (_monitored && !buffer(0)->empty()) {
				// Monitoring explictly enabled, send everything
				const LV2_Atom_Sequence* seq = (const LV2_Atom_Sequence*)atom;
				LV2_ATOM_SEQUENCE_FOREACH(seq, ev) {
					context.notify(uris.ingen_activity,
					               context.start() + ev->time.frames,
					               this,
					               ev->body.size,
					               ev->body.type,
					               LV2_ATOM_BODY(&ev->body));
				}
			} else if (!buffer(0)->empty()) {
				// Just send activity for blinkenlights
				const int32_t one = 1;
				context.notify(uris.ingen_activity,
				               context.start(),
				               this,
				               sizeof(int32_t),
				               (LV2_URID)uris.atom_Bool,
				               &one);
				_force_monitor_update = false;
			}
		}
	}

	_frames_since_monitor = _frames_since_monitor % period;
	if (key && val != _monitor_value) {
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
}

BufferRef
PortImpl::value_buffer(uint32_t voice)
{
	return buffer(voice)->value_buffer();
}

SampleCount
PortImpl::next_value_offset(SampleCount offset, SampleCount end) const
{
	return end;
}

} // namespace Server
} // namespace Ingen
