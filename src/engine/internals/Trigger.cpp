/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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

#include <cmath>
#include "raul/log.hpp"
#include "raul/midi_events.h"
#include "shared/LV2URIMap.hpp"
#include "internals/Trigger.hpp"
#include "AudioBuffer.hpp"
#include "EventBuffer.hpp"
#include "InputPort.hpp"
#include "InternalPlugin.hpp"
#include "OutputPort.hpp"
#include "ProcessContext.hpp"
#include "util.hpp"
#include "ingen-config.h"

#define LOG(s) s << "[TriggerNode] "

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Internals {

using namespace Shared;

static InternalPlugin trigger_plugin(NS_INTERNALS "Trigger", "trigger");

InternalPlugin& TriggerNode::internal_plugin() { return trigger_plugin; }

TriggerNode::TriggerNode(BufferFactory& bufs, const string& path, bool polyphonic, PatchImpl* parent, SampleRate srate)
	: NodeBase(&trigger_plugin, path, false, parent, srate)
	, _learning(false)
{
	const LV2URIMap& uris = LV2URIMap::instance();
	_ports = new Raul::Array<PortImpl*>(5);

	_midi_in_port = new InputPort(bufs, this, "input", 0, 1, PortType::EVENTS, Raul::Atom());
	_midi_in_port->set_property(uris.lv2_name, "Input");
	_ports->at(0) = _midi_in_port;

	_note_port = new InputPort(bufs, this, "note", 1, 1, PortType::CONTROL, 60.0f);
	_note_port->set_property(uris.lv2_minimum, 0.0f);
	_note_port->set_property(uris.lv2_maximum, 127.0f);
	_note_port->set_property(uris.lv2_integer, true);
	_note_port->set_property(uris.lv2_name, "Note");
	_ports->at(1) = _note_port;

	_gate_port = new OutputPort(bufs, this, "gate", 2, 1, PortType::AUDIO, 0.0f);
	_gate_port->set_property(uris.lv2_portProperty, uris.lv2_toggled);
	_gate_port->set_property(uris.lv2_name, "Gate");
	_ports->at(2) = _gate_port;

	_trig_port = new OutputPort(bufs, this, "trigger", 3, 1, PortType::AUDIO, 0.0f);
	_trig_port->set_property(uris.lv2_portProperty, uris.lv2_toggled);
	_trig_port->set_property(uris.lv2_name, "Trigger");
	_ports->at(3) = _trig_port;

	_vel_port = new OutputPort(bufs, this, "velocity", 4, 1, PortType::AUDIO, 0.0f);
	_vel_port->set_property(uris.lv2_minimum, 0.0f);
	_vel_port->set_property(uris.lv2_maximum, 1.0f);
	_vel_port->set_property(uris.lv2_name, "Velocity");
	_ports->at(4) = _vel_port;
}


void
TriggerNode::process(ProcessContext& context)
{
	NodeBase::pre_process(context);

	uint32_t frames = 0;
	uint32_t subframes = 0;
	uint16_t type = 0;
	uint16_t size = 0;
	uint8_t* buf = NULL;

	EventBuffer* const midi_in = (EventBuffer*)_midi_in_port->buffer(0).get();

	midi_in->rewind();

	while (midi_in->get_event(&frames, &subframes, &type, &size, &buf)) {
		const FrameTime time = context.start() + (FrameTime)frames;

		if (size >= 3) {
			switch (buf[0] & 0xF0) {
			case MIDI_CMD_NOTE_ON:
				if (buf[2] == 0)
					note_off(context, buf[1], time);
				else
					note_on(context, buf[1], buf[2], time);
				break;
			case MIDI_CMD_NOTE_OFF:
				note_off(context, buf[1], time);
				break;
			case MIDI_CMD_CONTROL:
				if (buf[1] == MIDI_CTL_ALL_NOTES_OFF
						|| buf[1] == MIDI_CTL_ALL_SOUNDS_OFF)
					((AudioBuffer*)_gate_port->buffer(0).get())->set_value(0.0f, context.start(), time);
			default:
				break;
			}
		}

		midi_in->increment();
	}

	NodeBase::post_process(context);
}


void
TriggerNode::note_on(ProcessContext& context, uint8_t note_num, uint8_t velocity, FrameTime time)
{
	assert(time >= context.start() && time <= context.end());

	if (_learning) {
		_note_port->set_value(note_num);
		((AudioBuffer*)_note_port->buffer(0).get())->set_value(
				(float)note_num, context.start(), context.end());
		_note_port->broadcast_value(context, true);
		_learning = false;
	}

#ifdef LOG_DEBUG
	LOG(debug) << path() << " note " << (int)note_num << " on @ " << time << endl;
#endif

	Sample filter_note = ((AudioBuffer*)_note_port->buffer(0).get())->value_at(0);
	if (filter_note >= 0.0 && filter_note < 127.0 && (note_num == (uint8_t)filter_note)) {
		((AudioBuffer*)_gate_port->buffer(0).get())->set_value(1.0f, context.start(), time);
		((AudioBuffer*)_trig_port->buffer(0).get())->set_value(1.0f, context.start(), time);
		((AudioBuffer*)_trig_port->buffer(0).get())->set_value(0.0f, context.start(), time + 1);
		((AudioBuffer*)_vel_port->buffer(0).get())->set_value(velocity / 127.0f, context.start(), time);
		assert(((AudioBuffer*)_trig_port->buffer(0).get())->data()[time - context.start()] == 1.0f);
	}
}


void
TriggerNode::note_off(ProcessContext& context, uint8_t note_num, FrameTime time)
{
	assert(time >= context.start() && time <= context.end());

	if (note_num == lrintf(((AudioBuffer*)_note_port->buffer(0).get())->value_at(0)))
		((AudioBuffer*)_gate_port->buffer(0).get())->set_value(0.0f, context.start(), time);
}


} // namespace Internals
} // namespace Ingen
