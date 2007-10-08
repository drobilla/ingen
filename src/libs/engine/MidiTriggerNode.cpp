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
#include "PluginImpl.hpp"
#include "ProcessContext.hpp"
#include "util.hpp"

namespace Ingen {


MidiTriggerNode::MidiTriggerNode(const string& path, bool polyphonic, PatchImpl* parent, SampleRate srate, size_t buffer_size)
: NodeBase(new PluginImpl(Plugin::Internal, "ingen:trigger_node"), path, false, parent, srate, buffer_size)
{
	_ports = new Raul::Array<PortImpl*>(5);

	_midi_in_port = new InputPort(this, "MIDIIn", 0, 1, DataType::MIDI, _buffer_size);
	_ports->at(0) = _midi_in_port;
	
	_note_port = new InputPort(this, "NoteNumber", 1, 1, DataType::CONTROL, 1);
	_note_port->set_variable("ingen:minimum", 0.0f);
	_note_port->set_variable("ingen:maximum", 127.0f);
	_note_port->set_variable("ingen:default", 60.0f);
	_note_port->set_variable("ingen:integer", 1);
	_ports->at(1) = _note_port;
	
	_gate_port = new OutputPort(this, "Gate", 2, 1, DataType::AUDIO, _buffer_size);
	_ports->at(2) = _gate_port;

	_trig_port = new OutputPort(this, "Trigger", 3, 1, DataType::AUDIO, _buffer_size);
	_ports->at(3) = _trig_port;
	
	_vel_port = new OutputPort(this, "Velocity", 4, 1, DataType::AUDIO, _buffer_size);
	_ports->at(4) = _vel_port;
	
	PluginImpl* p = const_cast<PluginImpl*>(_plugin);
	p->plug_label("trigger_in");
	assert(p->uri() == "ingen:trigger_node");
	p->name("Ingen Trigger Node (MIDI, OSC)");
}


void
MidiTriggerNode::process(ProcessContext& context)
{
	NodeBase::pre_process(context);
	
	double         timestamp = 0;
	uint32_t       size = 0;
	unsigned char* buffer = NULL;

	MidiBuffer* const midi_in = (MidiBuffer*)_midi_in_port->buffer(0);
	assert(midi_in->this_nframes() == context.nframes());

	while (midi_in->get_event(&timestamp, &size, &buffer) < context.nframes()) {
		
		const FrameTime time = context.start() + (FrameTime)timestamp;

		if (size >= 3) {
			switch (buffer[0] & 0xF0) {
			case MIDI_CMD_NOTE_ON:
				if (buffer[2] == 0)
					note_off(buffer[1], time, context);
				else
					note_on(buffer[1], buffer[2], time, context);
				break;
			case MIDI_CMD_NOTE_OFF:
				note_off(buffer[1], time, context);
				break;
			case MIDI_CMD_CONTROL:
				if (buffer[1] == MIDI_CTL_ALL_NOTES_OFF
						|| buffer[1] == MIDI_CTL_ALL_SOUNDS_OFF)
					((AudioBuffer*)_gate_port->buffer(0))->set(0.0f, time);
			default:
				break;
			}
		}
		
		midi_in->increment();
	}
	
	NodeBase::post_process(context);
}


void
MidiTriggerNode::note_on(uchar note_num, uchar velocity, FrameTime time, ProcessContext& context)
{
	assert(time >= context.start() && time <= context.end());
	assert(time - context.start() < _buffer_size);

	//std::cerr << "Note on starting at sample " << offset << std::endl;

	const Sample filter_note = ((AudioBuffer*)_note_port->buffer(0))->value_at(0);
	if (filter_note >= 0.0 && filter_note < 127.0 && (note_num == (uchar)filter_note)){
			
		// FIXME FIXME FIXME
		SampleCount offset = time - context.start();

		// See comments in MidiNoteNode::note_on (FIXME)
		if (offset == (SampleCount)(_buffer_size-1))
			--offset;
		
		((AudioBuffer*)_gate_port->buffer(0))->set(1.0f, offset);
		((AudioBuffer*)_trig_port->buffer(0))->set(1.0f, offset, offset);
		((AudioBuffer*)_trig_port->buffer(0))->set(0.0f, offset+1);
		((AudioBuffer*)_vel_port->buffer(0))->set(velocity/127.0f, offset);
	}
}


void
MidiTriggerNode::note_off(uchar note_num, FrameTime time, ProcessContext& context)
{
	assert(time >= context.start() && time <= context.end());
	assert(time - context.start() < _buffer_size);

	if (note_num == lrintf(((AudioBuffer*)_note_port->buffer(0))->value_at(0)))
		((AudioBuffer*)_gate_port->buffer(0))->set(0.0f, time - context.start());
}


} // namespace Ingen

