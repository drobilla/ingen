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

#include <math.h>

#include "ingen/Log.hpp"
#include "ingen/URIMap.hpp"
#include "ingen/URIs.hpp"
#include "ingen/World.hpp"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"

#include "Buffer.hpp"
#include "ControlBindings.hpp"
#include "Driver.hpp"
#include "Engine.hpp"
#include "PortImpl.hpp"
#include "RunContext.hpp"
#include "ThreadManager.hpp"

using namespace std;

namespace Ingen {
namespace Server {

ControlBindings::ControlBindings(Engine& engine)
	: _engine(engine)
	, _learn_binding(NULL)
	, _bindings(new Bindings())
	, _feedback(new Buffer(*_engine.buffer_factory(),
	                       engine.world()->uris().atom_Sequence,
	                       0,
	                       4096)) // FIXME: capacity?
{
	lv2_atom_forge_init(
		&_forge, &engine.world()->uri_map().urid_map_feature()->urid_map);
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
	const Ingen::URIs& uris    = _engine.world()->uris();
	const Atom&        binding = port->get_property(uris.midi_binding);
	return binding_key(binding);
}

ControlBindings::Key
ControlBindings::binding_key(const Atom& binding) const
{
	const Ingen::URIs& uris = _engine.world()->uris();
	Key       key;
	LV2_Atom* num = NULL;
	if (binding.type() == uris.atom_Object) {
		const LV2_Atom_Object_Body* obj = (const LV2_Atom_Object_Body*)
			binding.get_body();
		if (obj->otype == uris.midi_Bender) {
			key = Key(Type::MIDI_BENDER);
		} else if (obj->otype == uris.midi_ChannelPressure) {
			key = Key(Type::MIDI_CHANNEL_PRESSURE);
		} else if (obj->otype == uris.midi_Controller) {
			lv2_atom_object_body_get(
				binding.size(), obj, (LV2_URID)uris.midi_controllerNumber, &num, NULL);
			if (!num) {
				_engine.log().rt_error("Controller binding missing number\n");
			} else if (num->type != uris.atom_Int) {
				_engine.log().rt_error("Controller number not an integer\n");
			} else {
				key = Key(Type::MIDI_CC, ((LV2_Atom_Int*)num)->body);
			}
		} else if (obj->otype == uris.midi_NoteOn) {
			lv2_atom_object_body_get(
				binding.size(), obj, (LV2_URID)uris.midi_noteNumber, &num, NULL);
			if (!num) {
				_engine.log().rt_error("Note binding missing number\n");
			} else if (num->type != uris.atom_Int) {
				_engine.log().rt_error("Note number not an integer\n");
			} else {
				key = Key(Type::MIDI_NOTE, ((LV2_Atom_Int*)num)->body);
			}
		}
	} else if (binding.type()) {
		_engine.log().rt_error("Unknown binding type\n");
	}
	return key;
}

ControlBindings::Key
ControlBindings::midi_event_key(uint16_t size, const uint8_t* buf, uint16_t& value)
{
	switch (lv2_midi_message_type(buf)) {
	case LV2_MIDI_MSG_CONTROLLER:
		value = static_cast<const int8_t>(buf[2]);
		return Key(Type::MIDI_CC, static_cast<const int8_t>(buf[1]));
	case LV2_MIDI_MSG_BENDER:
		value = (static_cast<int8_t>(buf[2]) << 7) + static_cast<int8_t>(buf[1]);
		return Key(Type::MIDI_BENDER);
	case LV2_MIDI_MSG_CHANNEL_PRESSURE:
		value = static_cast<const int8_t>(buf[1]);
		return Key(Type::MIDI_CHANNEL_PRESSURE);
	case LV2_MIDI_MSG_NOTE_ON:
		value = 1.0f;
		return Key(Type::MIDI_NOTE, static_cast<const int8_t>(buf[1]));
	default:
		return Key();
	}
}

bool
ControlBindings::set_port_binding(RunContext& context,
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
	} else {
		return false;
	}
}

void
ControlBindings::port_value_changed(RunContext& context,
                                    PortImpl*   port,
                                    Key         key,
                                    const Atom& value_atom)
{
	Ingen::World*      world = context.engine().world();
	const Ingen::URIs& uris  = world->uris();
	if (!!key) {
		int16_t  value = port_value_to_control(
			context, port, key.type, value_atom);
		uint16_t size  = 0;
		uint8_t  buf[4];
		switch (key.type) {
		case Type::MIDI_CC:
			size = 3;
			buf[0] = LV2_MIDI_MSG_CONTROLLER;
			buf[1] = key.num;
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
			buf[1] = key.num;
			buf[2] = 0x64; // MIDI spec default
			break;
		default:
			break;
		}
		if (size > 0) {
			_feedback->append_event(context.nframes() - 1, size, (LV2_URID)uris.midi_MidiEvent, buf);
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
get_range(RunContext& context, const PortImpl* port, float* min, float* max)
{
	*min = port->minimum().get<float>();
	*max = port->maximum().get<float>();
	if (port->is_sample_rate()) {
		*min *= context.engine().driver()->sample_rate();
		*max *= context.engine().driver()->sample_rate();
	}
}

float
ControlBindings::control_to_port_value(RunContext&     context,
                                       const PortImpl* port,
                                       Type            type,
                                       int16_t         value) const
{
	float normal = 0.0f;
	switch (type) {
	case Type::MIDI_CC:
	case Type::MIDI_CHANNEL_PRESSURE:
		normal = (float)value / 127.0f;
		break;
	case Type::MIDI_BENDER:
		normal = (float)value / 16383.0f;
		break;
	case Type::MIDI_NOTE:
		normal = (value == 0.0f) ? 0.0f : 1.0f;
		break;
	default:
		break;
	}

	if (port->is_logarithmic()) {
		normal = (expf(normal) - 1.0f) / ((float)M_E - 1.0f);
	}

	float min, max;
	get_range(context, port, &min, &max);

	return normal * (max - min) + min;
}

int16_t
ControlBindings::port_value_to_control(RunContext& context,
                                       PortImpl*   port,
                                       Type        type,
                                       const Atom& value_atom) const
{
	if (value_atom.type() != port->bufs().forge().Float)
		return 0;

	float min, max;
	get_range(context, port, &min, &max);

	const float value  = value_atom.get<float>();
	float       normal = (value - min) / (max - min);

	if (normal < 0.0f) {
		normal = 0.0f;
	}

	if (normal > 1.0f) {
		normal = 1.0f;
	}

	if (port->is_logarithmic()) {
		normal = logf(normal * ((float)M_E - 1.0f) + 1.0);
	}

	switch (type) {
	case Type::MIDI_CC:
	case Type::MIDI_CHANNEL_PRESSURE:
		return lrintf(normal * 127.0f);
	case Type::MIDI_BENDER:
		return lrintf(normal * 16383.0f);
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
		break;
	case ControlBindings::Type::MIDI_CHANNEL_PRESSURE:
		lv2_atom_forge_object(forge, &frame, 0, uris.midi_ChannelPressure);
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
ControlBindings::set_port_value(RunContext& context,
                                PortImpl*   port,
                                Type        type,
                                int16_t     value)
{
	float min, max;
	get_range(context, port, &min, &max);

	const float val = control_to_port_value(context, port, type, value);

	// TODO: Set port value property so it is saved
	port->set_control_value(context, context.start(), val);

	URIs& uris = context.engine().world()->uris();
	context.notify(uris.ingen_value, context.start(), port,
	               sizeof(float), _forge.Float, &val);
}

bool
ControlBindings::finish_learn(RunContext& context, Key key)
{
	const Ingen::URIs& uris    = context.engine().world()->uris();
	Binding*           binding = _learn_binding.exchange(NULL);
	if (!binding || (key.type == Type::MIDI_NOTE && !binding->port->is_toggled())) {
		return false;
	}

	binding->key = key;
	_bindings->insert(*binding);

	uint8_t buf[128];
	memset(buf, 0, sizeof(buf));
	lv2_atom_forge_set_buffer(&_forge, buf, sizeof(buf));
	forge_binding(uris, &_forge, key.type, key.num);
	const LV2_Atom* atom = (const LV2_Atom*)buf;
	context.notify(uris.midi_binding,
	               context.start(),
	               binding->port,
	               atom->size, atom->type, LV2_ATOM_BODY_CONST(atom));

	return true;
}

void
ControlBindings::get_all(const Raul::Path& path, std::vector<Binding*>& bindings)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);

	for (Binding& b : *_bindings) {
		if (b.port->path() == path || b.port->path().is_child_of(path)) {
			bindings.push_back(&b);
		}
	}
}

void
ControlBindings::remove(RunContext& ctx, const std::vector<Binding*>& bindings)
{
	for (Binding* b : bindings) {
		_bindings->erase(*b);
	}
}

void
ControlBindings::pre_process(RunContext& context, Buffer* buffer)
{
	uint16_t           value = 0;
	Ingen::World*      world = context.engine().world();
	const Ingen::URIs& uris  = world->uris();

	_feedback->clear();
	if (!_learn_binding && _bindings->empty()) {
		return;  // Don't bother reading input
	}

	LV2_Atom_Sequence* seq = buffer->get<LV2_Atom_Sequence>();
	LV2_ATOM_SEQUENCE_FOREACH(seq, ev) {
		if (ev->body.type == uris.midi_MidiEvent) {
			const uint8_t* buf = (const uint8_t*)LV2_ATOM_BODY(&ev->body);
			const Key      key = midi_event_key(ev->body.size, buf, value);

			if (_learn_binding && !!key) {
				finish_learn(context, key);  // Learn new binding
			}

			// Set all controls bound to this key
			const Binding k = {key, NULL};
			for (Bindings::const_iterator i = _bindings->lower_bound(k);
			     i != _bindings->end() && i->key == key;
			     ++i) {
				set_port_value(context, i->port, key.type, value);
			}
		}
	}
}

void
ControlBindings::post_process(RunContext& context, Buffer* buffer)
{
	buffer->append_event_buffer(_feedback.get());
}

} // namespace Server
} // namespace Ingen
