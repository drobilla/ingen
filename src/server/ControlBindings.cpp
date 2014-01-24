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
#include "ProcessContext.hpp"
#include "ThreadManager.hpp"

using namespace std;

namespace Ingen {
namespace Server {

ControlBindings::ControlBindings(Engine& engine)
	: _engine(engine)
	, _learn_port(NULL)
	, _bindings(new Bindings())
	, _feedback(new Buffer(*_engine.buffer_factory(),
	                       engine.world()->uris().atom_Sequence,
	                       4096)) // FIXME: capacity?
{
	lv2_atom_forge_init(
		&_forge, &engine.world()->uri_map().urid_map_feature()->urid_map);
}

ControlBindings::~ControlBindings()
{
	_feedback.reset();
}

ControlBindings::Key
ControlBindings::port_binding(PortImpl* port) const
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);
	const Ingen::URIs& uris = _engine.world()->uris();
	const Atom& binding = port->get_property(uris.midi_binding);
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
				_engine.log().error("Controller binding missing number\n");
			} else if (num->type != uris.atom_Int) {
				_engine.log().error("Controller number not an integer\n");
			} else {
				key = Key(Type::MIDI_CC, ((LV2_Atom_Int*)num)->body);
			}
		} else if (obj->otype == uris.midi_NoteOn) {
			lv2_atom_object_body_get(
				binding.size(), obj, (LV2_URID)uris.midi_noteNumber, &num, NULL);
			if (!num) {
				_engine.log().error("Note binding missing number\n");
			} else if (num->type != uris.atom_Int) {
				_engine.log().error("Note number not an integer\n");
			} else {
				key = Key(Type::MIDI_NOTE, ((LV2_Atom_Int*)num)->body);
			}
		}
	} else if (binding.type()) {
		_engine.log().error(fmt("Unknown binding type %1%\n") % binding.type());
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

void
ControlBindings::port_binding_changed(ProcessContext& context,
                                      PortImpl*       port,
                                      const Atom&     binding)
{
	const Key key = binding_key(binding);
	if (key) {
		_bindings->insert(make_pair(key, port));
	}
}

void
ControlBindings::port_value_changed(ProcessContext& context,
                                    PortImpl*       port,
                                    Key             key,
                                    const Atom&     value_atom)
{
	Ingen::World*      world = context.engine().world();
	const Ingen::URIs& uris  = world->uris();
	if (key) {
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
			_feedback->append_event(0, size, uris.midi_MidiEvent.id, buf);
		}
	}
}

void
ControlBindings::learn(PortImpl* port)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);
	_learn_port = port;
}

static void
get_range(ProcessContext& context, const PortImpl* port, float* min, float* max)
{
	*min = port->minimum().get<float>();
	*max = port->maximum().get<float>();
	if (port->is_sample_rate()) {
		*min *= context.engine().driver()->sample_rate();
		*max *= context.engine().driver()->sample_rate();
	}
}

Atom
ControlBindings::control_to_port_value(ProcessContext& context,
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

	return _engine.world()->forge().make(normal * (max - min) + min);
}

int16_t
ControlBindings::port_value_to_control(ProcessContext& context,
                                       PortImpl*       port,
                                       Type            type,
                                       const Atom&     value_atom) const
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
ControlBindings::set_port_value(ProcessContext& context,
                                PortImpl*       port,
                                Type            type,
                                int16_t         value)
{
	float min, max;
	get_range(context, port, &min, &max);

	const Atom port_value(control_to_port_value(context, port, type, value));

	assert(port_value.type() == port->bufs().forge().Float);
	port->set_value(port_value);  // FIXME: not thread safe
	port->set_control_value(context, context.start(), port_value.get<float>());

	URIs& uris = context.engine().world()->uris();
	context.notify(uris.ingen_value, context.start(), port,
	               port_value.size(), port_value.type(), port_value.get_body());
}

bool
ControlBindings::bind(ProcessContext& context, Key key)
{
	const Ingen::URIs& uris = context.engine().world()->uris();
	assert(_learn_port);
	if (key.type == Type::MIDI_NOTE) {
		if (!_learn_port->is_toggled())
			return false;
	}

	_bindings->insert(make_pair(key, _learn_port));

	uint8_t buf[128];
	lv2_atom_forge_set_buffer(&_forge, buf, sizeof(buf));
	forge_binding(uris, &_forge, key.type, key.num);
	const LV2_Atom* atom = (const LV2_Atom*)buf;
	context.notify(uris.midi_binding,
	               context.start(),
	               _learn_port,
	               atom->size, atom->type, LV2_ATOM_BODY_CONST(atom));

	_learn_port = NULL;
	return true;
}

SPtr<ControlBindings::Bindings>
ControlBindings::remove(const Raul::Path& path)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);

	SPtr<Bindings> old_bindings(_bindings);
	SPtr<Bindings> copy(new Bindings(*_bindings.get()));

	for (Bindings::iterator i = copy->begin(); i != copy->end();) {
		Bindings::iterator next = i;
		++next;

		if (i->second->path() == path || i->second->path().is_child_of(path))
			copy->erase(i);

		i = next;
	}

	_bindings = copy;
	return old_bindings;
}

SPtr<ControlBindings::Bindings>
ControlBindings::remove(PortImpl* port)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);

	SPtr<Bindings> old_bindings(_bindings);
	SPtr<Bindings> copy(new Bindings(*_bindings.get()));

	for (Bindings::iterator i = copy->begin(); i != copy->end();) {
		Bindings::iterator next = i;
		++next;

		if (i->second == port)
			copy->erase(i);

		i = next;
	}

	_bindings = copy;
	return old_bindings;
}

void
ControlBindings::pre_process(ProcessContext& context, Buffer* buffer)
{
	uint16_t       value    = 0;
	SPtr<Bindings> bindings = _bindings;
	_feedback->clear();

	Ingen::World*      world = context.engine().world();
	const Ingen::URIs& uris  = world->uris();

	if (!_learn_port && bindings->empty()) {
		// Don't bother reading input
		return;
	}

	LV2_Atom_Sequence* seq = (LV2_Atom_Sequence*)buffer->atom();
	LV2_ATOM_SEQUENCE_FOREACH(seq, ev) {
		if (ev->body.type == uris.midi_MidiEvent) {
			const uint8_t* buf = (const uint8_t*)LV2_ATOM_BODY(&ev->body);
			const Key      key = midi_event_key(ev->body.size, buf, value);
			if (_learn_port && key) {
				bind(context, key);
			}

			Bindings::const_iterator i = bindings->find(key);
			if (i != bindings->end()) {
				set_port_value(context, i->second, key.type, value);
			}
		}
	}
}

void
ControlBindings::post_process(ProcessContext& context, Buffer* buffer)
{
	// TODO: merge buffer's existing contents (anything send to it in the graph)
	buffer->copy(context, _feedback.get());
}

} // namespace Server
} // namespace Ingen
