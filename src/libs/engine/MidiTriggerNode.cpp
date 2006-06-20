/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "MidiTriggerNode.h"
#include <cmath>
#include "InputPort.h"
#include "OutputPort.h"
#include "Plugin.h"
#include "util.h"
#include "midi.h"

namespace Om {


MidiTriggerNode::MidiTriggerNode(const string& path, size_t poly, Patch* parent, samplerate srate, size_t buffer_size)
: InternalNode(new Plugin(Plugin::Internal, "Om:TriggerNode"), path, 1, parent, srate, buffer_size)
{
	_ports = new Array<Port*>(5);

	_midi_in_port = new InputPort<MidiMessage>(this, "DataType::MIDI In", 0, 1, DataType::MIDI, _buffer_size);
	_ports->at(0) = _midi_in_port;
	
	_note_port = new InputPort<sample>(this, "Note Number", 1, 1, DataType::FLOAT, 1);
	//	new PortInfo("Note Number", CONTROL, INPUT, INTEGER, 60, 0, 127), 1);
	_ports->at(1) = _note_port;
	
	_gate_port = new OutputPort<sample>(this, "Gate", 2, 1, DataType::FLOAT, _buffer_size);
	//	new PortInfo("Gate", AUDIO, OUTPUT, 0, 0, 1), _buffer_size);
	_ports->at(2) = _gate_port;

	_trig_port = new OutputPort<sample>(this, "Trigger", 3, 1, DataType::FLOAT, _buffer_size);
	//	new PortInfo("Trigger", AUDIO, OUTPUT, 0, 0, 1), _buffer_size);
	_ports->at(3) = _trig_port;
	
	_vel_port = new OutputPort<sample>(this, "Velocity", 4, poly, DataType::FLOAT, _buffer_size);
	//	new PortInfo("Velocity", AUDIO, OUTPUT, 0, 0, 1), _buffer_size);
	_ports->at(4) = _vel_port;
	
	_plugin.plug_label("trigger_in");
	_plugin.name("Om Trigger Node (MIDI, OSC)");
}


void
MidiTriggerNode::process(samplecount nframes)
{
	InternalNode::process(nframes);
	
	MidiMessage ev;
	
	for (size_t i=0; i < _midi_in_port->buffer(0)->filled_size(); ++i) {
		ev = _midi_in_port->buffer(0)->value_at(i);

		switch (ev.buffer[0] & 0xF0) {
		case MIDI_CMD_NOTE_ON:
			if (ev.buffer[2] == 0)
				note_off(ev.buffer[1], ev.time);
			else
				note_on(ev.buffer[1], ev.buffer[2], ev.time);
			break;
		case MIDI_CMD_NOTE_OFF:
			note_off(ev.buffer[1], ev.time);
			break;
		case MIDI_CMD_CONTROL:
			if (ev.buffer[1] == MIDI_CTL_ALL_NOTES_OFF
					|| ev.buffer[1] == MIDI_CTL_ALL_SOUNDS_OFF)
				_gate_port->buffer(0)->set(0.0f, ev.time);
		default:
			break;
		}
	}
}


void
MidiTriggerNode::note_on(uchar note_num, uchar velocity, samplecount offset)
{
	//std::cerr << "Note on starting at sample " << offset << std::endl;
	assert(offset < _buffer_size);

	const sample filter_note = _note_port->buffer(0)->value_at(0);
	if (filter_note >= 0.0 && filter_note < 127.0 && (note_num == (uchar)filter_note)){
				
		// See comments in MidiNoteNode::note_on (FIXME)
		if (offset == (samplecount)(_buffer_size-1))
			--offset;
		
		_gate_port->buffer(0)->set(1.0f, offset);
		_trig_port->buffer(0)->set(1.0f, offset, offset);
		_trig_port->buffer(0)->set(0.0f, offset+1);
		_vel_port->buffer(0)->set(velocity/127.0f, offset);
	}
}


void
MidiTriggerNode::note_off(uchar note_num, samplecount offset)
{
	assert(offset < _buffer_size);

	if (note_num == lrintf(_note_port->buffer(0)->value_at(0)))
		_gate_port->buffer(0)->set(0.0f, offset);
}


} // namespace Om

