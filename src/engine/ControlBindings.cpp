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
#include "EventBuffer.hpp"
#include "PortImpl.hpp"
#include "ProcessContext.hpp"
#include "ThreadManager.hpp"

#define LOG(s) s << "[ControlBindings] "

using namespace std;
using namespace Raul;

namespace Ingen {


void
ControlBindings::learn(PortImpl* port)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);
	_learn_port = port;
}


void
ControlBindings::set_port_value(ProcessContext& context, PortImpl* port, int8_t cc_value)
{
	// TODO: cache these to avoid the lookup
	float min = port->get_property("lv2:minimum").get_float();
	float max = port->get_property("lv2:maximum").get_float();

	Raul::Atom value(static_cast<float>(((float)cc_value / 127.0) * (max - min) + min));
	port->set_value(value);

	const Events::SendPortValue ev(context.engine(), context.start(), port, true, 0,
			value.get_float());
	context.event_sink().write(sizeof(ev), &ev);
}


void
ControlBindings::bind(ProcessContext& context, int8_t cc_num)
{
	_bindings->insert(make_pair(cc_num, _learn_port));

	const Events::SendBinding ev(context.engine(), context.start(), _learn_port,
			MessageType(MessageType::MIDI_CC, cc_num));
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
		while (buffer->get_event(&frames, &subframes, &type, &size, &buf)) {
			if (type == _map->midi_event.id && (buf[0] & 0xF0) == MIDI_CMD_CONTROL) {
				const int8_t controller = static_cast<const int8_t>(buf[1]);
				bind(context, controller);
				break;
			}
			buffer->increment();
		}
	}

	if (!bindings->empty()) {
		buffer->rewind();
		while (buffer->get_event(&frames, &subframes, &type, &size, &buf)) {
			if (type == _map->midi_event.id && (buf[0] & 0xF0) == MIDI_CMD_CONTROL) {
				const int8_t controller = static_cast<const int8_t>(buf[1]);
				const int8_t value      = static_cast<const int8_t>(buf[2]);
				Bindings::const_iterator i = bindings->find(controller);
				if (i != bindings->end()) {
					set_port_value(context, i->second, value);
				}
			}
			buffer->increment();
		}
	}
}

}
