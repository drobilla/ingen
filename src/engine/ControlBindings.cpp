/* This file is part of Ingen.
 * Copyright (C) 2009 Dave Robillard <http://drobilla.net>
 *
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "raul/log.hpp"
#include "raul/midi_events.h"
#include "events/SendPortValue.hpp"
#include "events/SendBinding.hpp"
#include "ControlBindings.hpp"
#include "Engine.hpp"
#include "EventBuffer.hpp"
#include "PortImpl.hpp"
#include "ProcessContext.hpp"
#include "ThreadManager.hpp"

#define LOG(s) s << "[ControlBindings] "

using namespace std;
using namespace Raul;

namespace Ingen {


void
ControlBindings::update_port(ProcessContext& context, PortImpl* port)
{
	const Shared::LV2URIMap& uris = Shared::LV2URIMap::instance();
	const Raul::Atom& binding = port->get_property(uris.ingen_controlBinding);
	if (binding.type() == Atom::DICT) {
		Atom::DictValue::const_iterator t = binding.get_dict().find(uris.rdf_type);
		Atom::DictValue::const_iterator n;
		if (t != binding.get_dict().end()) {
			if (t->second == uris.midi_Bender) {
				_bindings->insert(make_pair(Key(MIDI_BENDER), port));
			} else if (t->second == uris.midi_Bender) {
				_bindings->insert(make_pair(Key(MIDI_BENDER), port));
			} else if (t->second == uris.midi_ChannelPressure) {
				_bindings->insert(make_pair(Key(MIDI_CHANNEL_PRESSURE), port));
			} else if (t->second == uris.midi_Controller) {
				n = binding.get_dict().find(uris.midi_controllerNumber);
				if (n != binding.get_dict().end())
					_bindings->insert(make_pair(Key(MIDI_CC, n->second.get_int32()), port));
			} else if (t->second == uris.midi_Note) {
				n = binding.get_dict().find(uris.midi_noteNumber);
				if (n != binding.get_dict().end())
					_bindings->insert(make_pair(Key(MIDI_NOTE, n->second.get_int32()), port));
			}
		}
	}
}


void
ControlBindings::learn(PortImpl* port)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);
	_learn_port = port;
}


void
ControlBindings::set_port_value(ProcessContext& context, PortImpl* port, Type type, int16_t value)
{
	// TODO: cache these to avoid the lookup
	const Shared::LV2URIMap& uris = Shared::LV2URIMap::instance();
	float min = port->get_property(uris.lv2_minimum).get_float();
	float max = port->get_property(uris.lv2_maximum).get_float();
	bool toggled = port->has_property(uris.lv2_portProperty, uris.lv2_toggled);

	float normal;
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
	if (toggled)
		scaled_value = (scaled_value < 0.5) ? 0.0 : 1.0;

	Raul::Atom atom(scaled_value);
	port->set_value(atom);

	const Events::SendPortValue ev(context.engine(), context.start(), port, true, 0,
			atom.get_float());
	context.event_sink().write(sizeof(ev), &ev);
}


void
ControlBindings::bind(ProcessContext& context, Type type, int16_t num)
{
	assert(_learn_port);
	_bindings->insert(make_pair(Key(type, num), _learn_port));

	const Events::SendBinding ev(context.engine(), context.start(), _learn_port, type, num);
	context.event_sink().write(sizeof(ev), &ev);

	_learn_port = NULL;
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


void
ControlBindings::process(ProcessContext& context, EventBuffer* buffer)
{
	uint32_t frames    = 0;
	uint32_t subframes = 0;
	uint16_t type      = 0;
	uint16_t size      = 0;
	uint8_t* buf       = NULL;

	SharedPtr<Bindings> bindings = _bindings;

	if (_learn_port) {
		buffer->rewind();
		int16_t num;
		while (buffer->get_event(&frames, &subframes, &type, &size, &buf)) {
			if (type == _map->midi_event.id) {
				switch (buf[0] & 0xF0) {
				case MIDI_CMD_CONTROL:
					num = static_cast<const int8_t>(buf[1]);
					bind(context, MIDI_CC, num);
					break;
				case MIDI_CMD_BENDER:
					bind(context, MIDI_BENDER);
					break;
				case MIDI_CMD_CHANNEL_PRESSURE:
					bind(context, MIDI_CHANNEL_PRESSURE);
					break;
				case MIDI_CMD_NOTE_ON:
					num = static_cast<const int8_t>(buf[1]);
					bind(context, MIDI_NOTE, num);
					break;
				default:
					break;
				}
			}
			if (!_learn_port)
				break;
			buffer->increment();
		}
	}

	if (!bindings->empty()) {
		buffer->rewind();
		while (buffer->get_event(&frames, &subframes, &type, &size, &buf)) {
			if (type == _map->midi_event.id) {
				if ((buf[0] & 0xF0) == MIDI_CMD_CONTROL) {
					const int8_t controller = static_cast<const int8_t>(buf[1]);
					Bindings::const_iterator i = bindings->find(Key(MIDI_CC, controller));
					if (i != bindings->end()) {
						const int8_t value = static_cast<const int8_t>(buf[2]);
						set_port_value(context, i->second, MIDI_CC, value);
					}
				} else if ((buf[0] & 0xF0) == MIDI_CMD_BENDER) {
					Bindings::const_iterator i = bindings->find(Key(MIDI_BENDER));
					if (i != bindings->end()) {
						const int8_t lsb    = static_cast<const int8_t>(buf[1]);
						const int8_t msb    = static_cast<const int8_t>(buf[2]);
						const int16_t value = (msb << 7) + lsb;
						set_port_value(context, i->second, MIDI_BENDER, value);
					}
				} else if ((buf[0] & 0xF0) == MIDI_CMD_CHANNEL_PRESSURE) {
					Bindings::const_iterator i = bindings->find(Key(MIDI_CHANNEL_PRESSURE));
					if (i != bindings->end()) {
						const int8_t value = static_cast<const int8_t>(buf[1]);
						set_port_value(context, i->second, MIDI_CHANNEL_PRESSURE, value);
					}
				} else if ((buf[0] & 0xF0) == MIDI_CMD_NOTE_ON) {
					const int8_t num = static_cast<const int8_t>(buf[1]);
					Bindings::const_iterator i = bindings->find(Key(MIDI_NOTE, num));
					if (i != bindings->end()) {
						set_port_value(context, i->second, MIDI_NOTE, 1.0f);
					}
				} else if ((buf[0] & 0xF0) == MIDI_CMD_NOTE_OFF) {
					const int8_t num = static_cast<const int8_t>(buf[1]);
					Bindings::const_iterator i = bindings->find(Key(MIDI_NOTE, num));
					if (i != bindings->end()) {
						set_port_value(context, i->second, MIDI_NOTE, 0.0f);
					}
				}
			}
			buffer->increment();
		}
	}
}

}
