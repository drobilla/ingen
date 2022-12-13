/*
  This file is part of Ingen.
  Copyright 2007-2017 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ControlBindings.hpp"

#include "Buffer.hpp"
#include "BufferFactory.hpp"
#include "Engine.hpp"
#include "PortImpl.hpp"
#include "RunContext.hpp"
#include "ThreadManager.hpp"

#include "ingen/Atom.hpp"
#include "ingen/Forge.hpp"
#include "ingen/Log.hpp"
#include "ingen/URIMap.hpp"
#include "ingen/URIs.hpp"
#include "ingen/World.hpp"
#include "lv2/atom/atom.h"
#include "lv2/atom/forge.h"
#include "lv2/atom/util.h"
#include "lv2/midi/midi.h"
#include "lv2/urid/urid.h"
#include "raul/Path.hpp"

#include <boost/intrusive/bstree.hpp>

#include <cmath>
#include <cstring>
#include <string>

namespace ingen {
namespace server {

ControlBindings::ControlBindings(Engine& engine)
	: _engine(engine)
	, _learn_binding(nullptr)
	, _bindings(new Bindings())
	, _feedback(new Buffer(*_engine.buffer_factory(),
	                       engine.world().uris().atom_Sequence,
	                       0,
	                       4096)) // FIXME: capacity?
	, _forge()
{
	lv2_atom_forge_init(&_forge, &engine.world().uri_map().urid_map());
}

ControlBindings::~ControlBindings()
{
	_feedback.reset();
	delete _learn_binding.load();
}

ControlBindings::Key
ControlBindings::port_binding(PortImpl* port) const
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);
	const ingen::URIs& uris    = _engine.world().uris();
	const Atom&        binding = port->get_property(uris.midi_binding);
	return binding_key(binding);
}

static int16_t
get_atom_num(const LV2_Atom* const atom)
{
	return static_cast<int16_t>(reinterpret_cast<const LV2_Atom_Int*>(atom)->body);
}

ControlBindings::Key
ControlBindings::binding_key(const Atom& binding) const
{
	const ingen::URIs& uris = _engine.world().uris();
	Key       key;
	LV2_Atom* num = nullptr;
	if (binding.type() == uris.atom_Object) {
		const auto* obj = static_cast<const LV2_Atom_Object_Body*>(binding.get_body());
		if (obj->otype == uris.midi_Bender) {
			lv2_atom_object_body_get(binding.size(),
			                         obj,
			                         uris.midi_channel.urid(),
			                         &num,
			                         nullptr);
			if (!num) {
				_engine.log().rt_error("Bender binding missing channel\n");
			} else if (num->type != uris.atom_Int) {
				_engine.log().rt_error("Bender channel not an integer\n");
			} else {
				key = Key(Type::MIDI_BENDER, get_atom_num(num));
			}
		} else if (obj->otype == uris.midi_ChannelPressure) {
			lv2_atom_object_body_get(binding.size(),
			                         obj,
			                         uris.midi_channel.urid(),
			                         &num,
			                         nullptr);
			if (!num) {
				_engine.log().rt_error("Pressure binding missing channel\n");
			} else if (num->type != uris.atom_Int) {
				_engine.log().rt_error("Pressure channel not an integer\n");
			} else {
				key = Key(Type::MIDI_CHANNEL_PRESSURE, get_atom_num(num));
			}
		} else if (obj->otype == uris.midi_Controller) {
			lv2_atom_object_body_get(binding.size(),
			                         obj,
			                         uris.midi_controllerNumber.urid(),
			                         &num,
			                         nullptr);
			if (!num) {
				_engine.log().rt_error("Controller binding missing number\n");
			} else if (num->type != uris.atom_Int) {
				_engine.log().rt_error("Controller number not an integer\n");
			} else {
				key = Key(Type::MIDI_CC, get_atom_num(num));
			}
		} else if (obj->otype == uris.midi_NoteOn) {
			lv2_atom_object_body_get(binding.size(),
			                         obj,
			                         uris.midi_noteNumber.urid(),
			                         &num,
			                         nullptr);
			if (!num) {
				_engine.log().rt_error("Note binding missing number\n");
			} else if (num->type != uris.atom_Int) {
				_engine.log().rt_error("Note number not an integer\n");
			} else {
				key = Key(Type::MIDI_NOTE, get_atom_num(num));
			}
		}
	} else if (binding.type()) {
		_engine.log().rt_error("Unknown binding type\n");
	}
	return key;
}

ControlBindings::Key
ControlBindings::midi_event_key(const uint8_t* buf, uint16_t& value)
{
	switch (lv2_midi_message_type(buf)) {
	case LV2_MIDI_MSG_CONTROLLER:
		value = buf[2];
		return {Type::MIDI_CC,
		        static_cast<int16_t>(((buf[0] & 0x0FU) << 8U) | buf[1])};
	case LV2_MIDI_MSG_BENDER:
		value = static_cast<uint16_t>((buf[2] << 7U) + buf[1]);
		return {Type::MIDI_BENDER, static_cast<int16_t>((buf[0] & 0x0FU))};
	case LV2_MIDI_MSG_CHANNEL_PRESSURE:
		value = buf[1];
		return {Type::MIDI_CHANNEL_PRESSURE,
		        static_cast<int16_t>((buf[0] & 0x0FU))};
	case LV2_MIDI_MSG_NOTE_ON:
		value = 1;
		return {Type::MIDI_NOTE,
		        static_cast<int16_t>(((buf[0] & 0x0FU) << 8U) | buf[1])};
	case LV2_MIDI_MSG_NOTE_OFF:
		value = 0;
		return {Type::MIDI_NOTE,
		        static_cast<int16_t>(((buf[0] & 0x0FU) << 8U) | buf[1])};
	default:
		return {};
	}
}

bool
ControlBindings::set_port_binding(RunContext&,
                                  PortImpl*   port,
                                  Binding*    binding,
                                  const Atom& value)
{
	const Key key = binding_key(value);
	if (!!key) {
		binding->key  = key;
		binding->port = port;
		_bindings->insert(*binding);
		return true;
	}

	return false;
}

void
ControlBindings::port_value_changed(RunContext& ctx,
                                    PortImpl*   port,
                                    Key         key,
                                    const Atom& value_atom)
{
	const ingen::URIs& uris = ctx.engine().world().uris();
	if (!!key) {
		int16_t  value = port_value_to_control(
			ctx, port, key.type, value_atom);
		uint16_t size  = 0;
		uint8_t  buf[4];
		switch (key.type) {
		case Type::MIDI_CC:
			size = 3;
			buf[0] = LV2_MIDI_MSG_CONTROLLER;
			buf[1] = static_cast<uint8_t>(key.num);
			buf[2] = static_cast<int8_t>(value);
			break;
		case Type::MIDI_CHANNEL_PRESSURE:
			size = 2;
			buf[0] = LV2_MIDI_MSG_CHANNEL_PRESSURE;
			buf[1] = static_cast<int8_t>(value);
			break;
		case Type::MIDI_BENDER:
			size = 3;
			buf[0] = LV2_MIDI_MSG_BENDER;
			buf[1] = (value & 0x007F);
			buf[2] = (value & 0x7F00) >> 7;
			break;
		case Type::MIDI_NOTE:
			size = 3;
			if (value == 1) {
				buf[0] = LV2_MIDI_MSG_NOTE_ON;
			} else if (value == 0) {
				buf[0] = LV2_MIDI_MSG_NOTE_OFF;
			}
			buf[1] = static_cast<uint8_t>(key.num);
			buf[2] = 0x64; // MIDI spec default
			break;
		default:
			break;
		}
		if (size > 0) {
			_feedback->append_event(ctx.nframes() - 1,
			                        size,
			                        static_cast<LV2_URID>(uris.midi_MidiEvent),
			                        buf);
		}
	}
}

void
ControlBindings::start_learn(PortImpl* port)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);
	Binding* b = _learn_binding.load();
	if (!b) {
		_learn_binding = new Binding(Type::NULL_CONTROL, port);
	} else {
		b->port = port;
	}
}

static void
get_range(RunContext& ctx, const PortImpl* port, float* min, float* max)
{
	*min = port->minimum().get<float>();
	*max = port->maximum().get<float>();
	if (port->is_sample_rate()) {
		*min *= ctx.engine().sample_rate();
		*max *= ctx.engine().sample_rate();
	}
}

float
ControlBindings::control_to_port_value(RunContext&     ctx,
                                       const PortImpl* port,
                                       Type            type,
                                       int16_t         value)
{
	float normal = 0.0f;
	switch (type) {
	case Type::MIDI_CC:
	case Type::MIDI_CHANNEL_PRESSURE:
		normal = static_cast<float>(value) / 127.0f;
		break;
	case Type::MIDI_BENDER:
		normal = static_cast<float>(value) / 16383.0f;
		break;
	case Type::MIDI_NOTE:
		normal = (value == 0) ? 0.0f : 1.0f;
		break;
	default:
		break;
	}

	if (port->is_logarithmic()) {
		normal = (expf(normal) - 1.0f) / (static_cast<float>(M_E) - 1.0f);
	}

	float min = 0.0f;
	float max = 1.0f;
	get_range(ctx, port, &min, &max);

	return normal * (max - min) + min;
}

int16_t
ControlBindings::port_value_to_control(RunContext& ctx,
                                       PortImpl*   port,
                                       Type        type,
                                       const Atom& value_atom)
{
	if (value_atom.type() != port->bufs().forge().Float) {
		return 0;
	}

	float min = 0.0f;
	float max = 1.0f;
	get_range(ctx, port, &min, &max);

	const float value  = value_atom.get<float>();
	float       normal = (value - min) / (max - min);

	if (normal < 0.0f) {
		normal = 0.0f;
	}

	if (normal > 1.0f) {
		normal = 1.0f;
	}

	if (port->is_logarithmic()) {
		normal = logf(normal * (static_cast<float>(M_E) - 1.0f) + 1.0f);
	}

	switch (type) {
	case Type::MIDI_CC:
	case Type::MIDI_CHANNEL_PRESSURE:
		return static_cast<int16_t>(lrintf(normal * 127.0f));
	case Type::MIDI_BENDER:
		return static_cast<int16_t>(lrintf(normal * 16383.0f));
	case Type::MIDI_NOTE:
		return (value > 0.0f) ? 1 : 0;
	default:
		return 0;
	}
}

static void
forge_binding(const URIs&           uris,
              LV2_Atom_Forge*       forge,
              ControlBindings::Type binding_type,
              int32_t               value)
{
	LV2_Atom_Forge_Frame frame;
	switch (binding_type) {
	case ControlBindings::Type::MIDI_CC:
		lv2_atom_forge_object(forge, &frame, 0, uris.midi_Controller);
		lv2_atom_forge_key(forge, uris.midi_controllerNumber);
		lv2_atom_forge_int(forge, value);
		break;
	case ControlBindings::Type::MIDI_BENDER:
		lv2_atom_forge_object(forge, &frame, 0, uris.midi_Bender);
		lv2_atom_forge_key(forge, uris.midi_channel);
		lv2_atom_forge_int(forge, value);
		break;
	case ControlBindings::Type::MIDI_CHANNEL_PRESSURE:
		lv2_atom_forge_object(forge, &frame, 0, uris.midi_ChannelPressure);
		lv2_atom_forge_key(forge, uris.midi_channel);
		lv2_atom_forge_int(forge, value);
		break;
	case ControlBindings::Type::MIDI_NOTE:
		lv2_atom_forge_object(forge, &frame, 0, uris.midi_NoteOn);
		lv2_atom_forge_key(forge, uris.midi_noteNumber);
		lv2_atom_forge_int(forge, value);
		break;
	case ControlBindings::Type::MIDI_RPN: // TODO
	case ControlBindings::Type::MIDI_NRPN: // TODO
	case ControlBindings::Type::NULL_CONTROL:
		break;
	}
}

void
ControlBindings::set_port_value(RunContext& ctx,
                                PortImpl*   port,
                                Type        type,
                                int16_t     value) const
{
	float min = 0.0f;
	float max = 1.0f;
	get_range(ctx, port, &min, &max);

	const float val = control_to_port_value(ctx, port, type, value);

	// TODO: Set port value property so it is saved
	port->set_control_value(ctx, ctx.start(), val);

	URIs& uris = ctx.engine().world().uris();
	ctx.notify(uris.ingen_value, ctx.start(), port,
	           sizeof(float), _forge.Float, &val);
}

bool
ControlBindings::finish_learn(RunContext& ctx, Key key)
{
	const ingen::URIs& uris    = ctx.engine().world().uris();
	Binding*           binding = _learn_binding.exchange(nullptr);
	if (!binding || (key.type == Type::MIDI_NOTE && !binding->port->is_toggled())) {
		return false;
	}

	binding->key = key;
	_bindings->insert(*binding);

	LV2_Atom buf[16];
	memset(buf, 0, sizeof(buf));
	lv2_atom_forge_set_buffer(&_forge, reinterpret_cast<uint8_t*>(buf), sizeof(buf));
	forge_binding(uris, &_forge, key.type, key.num);
	const LV2_Atom* atom = buf;
	ctx.notify(uris.midi_binding,
	           ctx.start(),
	           binding->port,
	           atom->size, atom->type, LV2_ATOM_BODY_CONST(atom));

	return true;
}

void
ControlBindings::get_all(const raul::Path& path, std::vector<Binding*>& bindings)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);

	for (Binding& b : *_bindings) {
		if (b.port->path() == path || b.port->path().is_child_of(path)) {
			bindings.push_back(&b);
		}
	}
}

void
ControlBindings::remove(RunContext&, const std::vector<Binding*>& bindings)
{
	for (Binding* b : bindings) {
		_bindings->erase(*b);
	}
}

void
ControlBindings::pre_process(RunContext& ctx, Buffer* buffer)
{
	uint16_t           value = 0;
	const ingen::URIs& uris  = ctx.engine().world().uris();

	_feedback->clear();
	if ((!_learn_binding && _bindings->empty()) || !buffer->get<LV2_Atom>()) {
		return; // Don't bother reading input
	}

	auto* seq = buffer->get<LV2_Atom_Sequence>();
	LV2_ATOM_SEQUENCE_FOREACH (seq, ev) {
		if (ev->body.type == uris.midi_MidiEvent) {
			const auto* buf = static_cast<const uint8_t*>(LV2_ATOM_BODY(&ev->body));
			const Key   key = midi_event_key(buf, value);

			if (_learn_binding && !!key) {
				finish_learn(ctx, key); // Learn new binding
			}

			// Set all controls bound to this key
			const Binding k = {key, nullptr};
			for (auto i = _bindings->lower_bound(k);
			     i != _bindings->end() && i->key == key;
			     ++i) {
				set_port_value(ctx, i->port, key.type, value);
			}
		}
	}
}

void
ControlBindings::post_process(RunContext&, Buffer* buffer)
{
	if (buffer->get<LV2_Atom>()) {
		buffer->append_event_buffer(_feedback.get());
	}
}

} // namespace server
} // namespace ingen
