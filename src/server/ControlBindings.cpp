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

#include <algorithm>

#include "ingen/shared/URIs.hpp"
#include "ingen/shared/World.hpp"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "raul/log.hpp"
#include "raul/midi_events.h"

#include "AudioBuffer.hpp"
#include "ControlBindings.hpp"
#include "Engine.hpp"
#include "Notification.hpp"
#include "PortImpl.hpp"
#include "ProcessContext.hpp"
#include "ThreadManager.hpp"

#define LOG(s) s << "[ControlBindings] "

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
}

ControlBindings::~ControlBindings()
{
	_feedback.reset();
}

ControlBindings::Key
ControlBindings::port_binding(PortImpl* port) const
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);
	const Ingen::Shared::URIs& uris = _engine.world()->uris();
	const Raul::Atom& binding = port->get_property(uris.ingen_controlBinding);
	return binding_key(binding);
}

ControlBindings::Key
ControlBindings::binding_key(const Raul::Atom& binding) const
{
	const Ingen::Shared::URIs& uris = _engine.world()->uris();
	Key key;
	if (binding.type() == _engine.world()->forge().Dict) {
		const Raul::Atom::DictValue&          dict = binding.get_dict();
		Raul::Atom::DictValue::const_iterator t    = dict.find(uris.rdf_type);
		Raul::Atom::DictValue::const_iterator n;
		if (t == dict.end()) {
			return key;
		} else if (t->second == uris.midi_Bender) {
			key = Key(MIDI_BENDER);
		} else if (t->second == uris.midi_ChannelPressure) {
			key = Key(MIDI_CHANNEL_PRESSURE);
		} else if (t->second == uris.midi_Controller) {
			if ((n = dict.find(uris.midi_controllerNumber)) != dict.end())
				key = Key(MIDI_CC, n->second.get_int32());
		} else if (t->second == uris.midi_NoteOn) {
			if ((n = dict.find(uris.midi_noteNumber)) != dict.end())
				key = Key(MIDI_NOTE, n->second.get_int32());
		}
	}
	return key;
}

ControlBindings::Key
ControlBindings::midi_event_key(uint16_t size, const uint8_t* buf, uint16_t& value)
{
	switch (buf[0] & 0xF0) {
	case MIDI_CMD_CONTROL:
		value = static_cast<const int8_t>(buf[2]);
		return Key(MIDI_CC, static_cast<const int8_t>(buf[1]));
	case MIDI_CMD_BENDER:
		value = (static_cast<int8_t>(buf[2]) << 7) + static_cast<int8_t>(buf[1]);
		return Key(MIDI_BENDER);
	case MIDI_CMD_CHANNEL_PRESSURE:
		value = static_cast<const int8_t>(buf[1]);
		return Key(MIDI_CHANNEL_PRESSURE);
	case MIDI_CMD_NOTE_ON:
		value = 1.0f;
		return Key(MIDI_NOTE, static_cast<const int8_t>(buf[1]));
	default:
		return Key();
	}
}

void
ControlBindings::port_binding_changed(ProcessContext&   context,
                                      PortImpl*         port,
                                      const Raul::Atom& binding)
{
	const Key key = binding_key(binding);
	if (key) {
		_bindings->insert(make_pair(key, port));
	}
}

void
ControlBindings::port_value_changed(ProcessContext&   context,
                                    PortImpl*         port,
                                    Key               key,
                                    const Raul::Atom& value_atom)
{
	Ingen::Shared::World*      world = context.engine().world();
	const Ingen::Shared::URIs& uris  = world->uris();
	if (key) {
		int16_t value = port_value_to_control(
			port, key.type, value_atom, port->minimum(), port->maximum());
		uint16_t size  = 0;
		uint8_t  buf[4];
		switch (key.type) {
		case MIDI_CC:
			size = 3;
			buf[0] = MIDI_CMD_CONTROL;
			buf[1] = key.num;
			buf[2] = static_cast<int8_t>(value);
			break;
		case MIDI_CHANNEL_PRESSURE:
			size = 2;
			buf[0] = MIDI_CMD_CHANNEL_PRESSURE;
			buf[1] = static_cast<int8_t>(value);
			break;
		case MIDI_BENDER:
			size = 3;
			buf[0] = MIDI_CMD_BENDER;
			buf[1] = (value & 0x007F);
			buf[2] = (value & 0x7F00) >> 7;
			break;
		case MIDI_NOTE:
			size = 3;
			if (value == 1)
				buf[0] = MIDI_CMD_NOTE_ON;
			else if (value == 0)
				buf[0] = MIDI_CMD_NOTE_OFF;
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

Raul::Atom
ControlBindings::control_to_port_value(Type              type,
                                       int16_t           value,
                                       const Raul::Atom& min_atom,
                                       const Raul::Atom& max_atom) const
{
	float min = min_atom.get_float();
	float max = max_atom.get_float();
	//bool  toggled = port->has_property(uris.lv2_portProperty, uris.lv2_toggled);

	float normal = 0.0f;
	switch (type) {
	case MIDI_CC:
	case MIDI_CHANNEL_PRESSURE:
		normal = (float)value / 127.0f;
		break;
	case MIDI_BENDER:
		normal = (float)value / 16383.0f;
		break;
	case MIDI_NOTE:
		normal = (value == 0.0f) ? 0.0f : 1.0f;
		break;
	default:
		break;
	}

	float scaled_value = normal * (max - min) + min;
	//if (toggled)
	//	scaled_value = (scaled_value < 0.5) ? 0.0 : 1.0;

	return _engine.world()->forge().make(scaled_value);
}

int16_t
ControlBindings::port_value_to_control(PortImpl*         port,
                                       Type              type,
                                       const Raul::Atom& value_atom,
                                       const Raul::Atom& min_atom,
                                       const Raul::Atom& max_atom) const
{
	if (value_atom.type() != port->bufs().forge().Float)
		return 0;

	const float min    = min_atom.get_float();
	const float max    = max_atom.get_float();
	const float value  = value_atom.get_float();
	float       normal = (value - min) / (max - min);

	if (normal < 0.0f) {
		LOG(Raul::warn) << "Value " << value << " (normal " << normal << ") for "
		                << port->path() << " out of range" << endl;
		normal = 0.0f;
	}

	if (normal > 1.0f) {
		LOG(Raul::warn) << "Value " << value << " (normal " << normal << ") for "
		                << port->path() << " out of range" << endl;
		normal = 1.0f;
	}

	switch (type) {
	case MIDI_CC:
	case MIDI_CHANNEL_PRESSURE:
		return lrintf(normal * 127.0f);
	case MIDI_BENDER:
		return lrintf(normal * 16383.0f);
	case MIDI_NOTE:
		return (value > 0.0f) ? 1 : 0;
	default:
		return 0;
	}
}

void
ControlBindings::set_port_value(ProcessContext& context,
                                PortImpl*       port,
                                Type            type,
                                int16_t         value)
{
	const Raul::Atom port_value(
		control_to_port_value(type, value, port->minimum(), port->maximum()));

	port->set_value(port_value);

	assert(port_value.type() == port->bufs().forge().Float);
	assert(dynamic_cast<AudioBuffer*>(port->buffer(0).get()));

	for (uint32_t v = 0; v < port->poly(); ++v)
		reinterpret_cast<AudioBuffer*>(port->buffer(v).get())->set_value(
			port_value.get_float(), context.start(), context.start());

	const Notification note = Notification::make(
		Notification::PORT_VALUE, context.start(), port, port_value);
	context.event_sink().write(sizeof(note), &note);
}

bool
ControlBindings::bind(ProcessContext& context, Key key)
{
	const Ingen::Shared::URIs& uris = context.engine().world()->uris();
	assert(_learn_port);
	if (key.type == MIDI_NOTE) {
		bool toggled = _learn_port->has_property(uris.lv2_portProperty, uris.lv2_toggled);
		if (!toggled)
			return false;
	}

	_bindings->insert(make_pair(key, _learn_port));

	const Notification note = Notification::make(
		Notification::PORT_BINDING, context.start(), _learn_port,
		context.engine().world()->forge().make(key.num), key.type);
	context.event_sink().write(sizeof(note), &note);

	_learn_port = NULL;
	return true;
}

SharedPtr<ControlBindings::Bindings>
ControlBindings::remove(const Raul::Path& path)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);

	SharedPtr<Bindings> old_bindings(_bindings);
	SharedPtr<Bindings> copy(new Bindings(*_bindings.get()));

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

SharedPtr<ControlBindings::Bindings>
ControlBindings::remove(PortImpl* port)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);

	SharedPtr<Bindings> old_bindings(_bindings);
	SharedPtr<Bindings> copy(new Bindings(*_bindings.get()));

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
	uint16_t            value    = 0;
	SharedPtr<Bindings> bindings = _bindings;
	_feedback->clear();

	Ingen::Shared::World*      world = context.engine().world();
	const Ingen::Shared::URIs& uris  = world->uris();

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
	// TODO: merge buffer's existing contents (anything send to it in the patch)
	buffer->copy(context, _feedback.get());
}

} // namespace Server
} // namespace Ingen
