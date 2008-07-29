/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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
#include <raul/midi_events.h>
#include "MidiTriggerNode.hpp"
#include "AudioBuffer.hpp"
#include "InputPort.hpp"
#include "OutputPort.hpp"
#include "InternalPlugin.hpp"
#include "ProcessContext.hpp"
#include "EventBuffer.hpp"
#include "util.hpp"

using namespace std;

namespace Ingen {


MidiTriggerNode::MidiTriggerNode(const string& path, bool polyphonic, PatchImpl* parent, SampleRate srate, size_t buffer_size)
	: NodeBase(new InternalPlugin("ingen:trigger_node", "trigger", "MIDI Trigger"),
			path, false, parent, srate, buffer_size)
{
	_ports = new Raul::Array<PortImpl*>(5);

	_midi_in_port = new InputPort(this, "input", 0, 1, DataType::EVENT, Atom(), _buffer_size);
	_ports->at(0) = _midi_in_port;
	
	_note_port = new InputPort(this, "note", 1, 1, DataType::CONTROL, 60.0f, 1);
	_note_port->set_variable("ingen:minimum", 0.0f);
	_note_port->set_variable("ingen:maximum", 127.0f);
	_note_port->set_variable("ingen:integer", true);
	_ports->at(1) = _note_port;
	
	_gate_port = new OutputPort(this, "gate", 2, 1, DataType::AUDIO, 0.0f, _buffer_size);
	_ports->at(2) = _gate_port;

	_trig_port = new OutputPort(this, "trigger", 3, 1, DataType::AUDIO, 0.0f, _buffer_size);
	_ports->at(3) = _trig_port;
	
	_vel_port = new OutputPort(this, "velocity", 4, 1, DataType::AUDIO, 0.0f, _buffer_size);
	_ports->at(4) = _vel_port;
}


void
MidiTriggerNode::process(ProcessContext& context)
{
	NodeBase::pre_process(context);
	
	uint32_t frames = 0;
	uint32_t subframes = 0;
	uint16_t type = 0;
	uint16_t size = 0;
	uint8_t* buf = NULL;

	EventBuffer* const midi_in = (EventBuffer*)_midi_in_port->buffer(0);
	assert(midi_in->this_nframes() == context.nframes());

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
					((AudioBuffer*)_gate_port->buffer(0))->set_value(0.0f, context.start(), time);
			default:
				break;
			}
		}
		
		midi_in->increment();
	}
	
	NodeBase::post_process(context);
}


void
MidiTriggerNode::note_on(ProcessContext& context, uchar note_num, uchar velocity, FrameTime time)
{
	assert(time >= context.start() && time <= context.end());
	assert(time - context.start() < _buffer_size);

	cerr << "[MidiTriggerNode] " << path() << " Note " << (int)note_num << " on @ " << time << endl;

	Sample filter_note = ((AudioBuffer*)_note_port->buffer(0))->value_at(0);
	cerr << "note: " << (int)note_num << ", filter: " << filter_note << endl;
	if (filter_note >= 0.0 && filter_note < 127.0 && (note_num == (uchar)filter_note)) {
			
		cerr << "!\n";

		((AudioBuffer*)_gate_port->buffer(0))->set_value(1.0f, context.start(), time);
		((AudioBuffer*)_trig_port->buffer(0))->set_value(1.0f, context.start(), time);
		((AudioBuffer*)_trig_port->buffer(0))->set_value(0.0f, context.start(), time + 1);
		((AudioBuffer*)_vel_port->buffer(0))->set_value(velocity / 127.0f, context.start(), time);
		
		assert(((AudioBuffer*)_trig_port->buffer(0))->data()[time - context.start()] == 1.0f);
	}
}


void
MidiTriggerNode::note_off(ProcessContext& context, uchar note_num, FrameTime time)
{
	assert(time >= context.start() && time <= context.end());
	assert(time - context.start() < _buffer_size);

	if (note_num == lrintf(((AudioBuffer*)_note_port->buffer(0))->value_at(0)))
		((AudioBuffer*)_gate_port->buffer(0))->set_value(0.0f, context.start(), time);
}


} // namespace Ingen

