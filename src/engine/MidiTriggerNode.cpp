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
#include "PortInfo.h"
#include "Plugin.h"
#include "util.h"
#include "midi.h"

namespace Om {


MidiTriggerNode::MidiTriggerNode(const string& path, size_t poly, Patch* parent, samplerate srate, size_t buffer_size)
: InternalNode(path, 1, parent, srate, buffer_size)
{
	m_num_ports = 5;
	m_ports.alloc(m_num_ports);

	m_midi_in_port = new InputPort<MidiMessage>(this, "MIDI In", 0, 1,
		new PortInfo("MIDI In", MIDI, INPUT), m_buffer_size);
	m_ports.at(0) = m_midi_in_port;
	
	m_note_port = new InputPort<sample>(this, "Note Number", 1, 1,
		new PortInfo("Note Number", CONTROL, INPUT, INTEGER, 60, 0, 127), 1);
	m_ports.at(1) = m_note_port;
	
	m_gate_port = new OutputPort<sample>(this, "Gate", 2, 1,
		new PortInfo("Gate", AUDIO, OUTPUT, 0, 0, 1), m_buffer_size);
	m_ports.at(2) = m_gate_port;

	m_trig_port = new OutputPort<sample>(this, "Trigger", 3, 1,
		new PortInfo("Trigger", AUDIO, OUTPUT, 0, 0, 1), m_buffer_size);
	m_ports.at(3) = m_trig_port;
	
	m_vel_port = new OutputPort<sample>(this, "Velocity", 4, poly,
		new PortInfo("Velocity", AUDIO, OUTPUT, 0, 0, 1), m_buffer_size);
	m_ports.at(4) = m_vel_port;
	
	m_plugin.type(Plugin::Internal);
	m_plugin.plug_label("trigger_in");
	m_plugin.name("Om Trigger Node (MIDI, OSC)");
}


void
MidiTriggerNode::run(size_t nframes)
{
	InternalNode::run(nframes);
	
	MidiMessage ev;
	
	for (size_t i=0; i < m_midi_in_port->buffer(0)->filled_size(); ++i) {
		ev = m_midi_in_port->buffer(0)->value_at(i);

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
				m_gate_port->buffer(0)->set(0.0f, ev.time);
		default:
			break;
		}
	}
}


void
MidiTriggerNode::note_on(uchar note_num, uchar velocity, samplecount offset)
{
	//std::cerr << "Note on starting at sample " << offset << std::endl;
	assert(offset < m_buffer_size);

	const sample filter_note = m_note_port->buffer(0)->value_at(0);
	if (filter_note >= 0.0 && filter_note < 127.0 && (note_num == (uchar)filter_note)){
				
		// See comments in MidiNoteNode::note_on (FIXME)
		if (offset == (samplecount)(m_buffer_size-1))
			--offset;
		
		m_gate_port->buffer(0)->set(1.0f, offset);
		m_trig_port->buffer(0)->set(1.0f, offset, offset);
		m_trig_port->buffer(0)->set(0.0f, offset+1);
		m_vel_port->buffer(0)->set(velocity/127.0f, offset);
	}
}


void
MidiTriggerNode::note_off(uchar note_num, samplecount offset)
{
	assert(offset < m_buffer_size);

	if (note_num == lrintf(m_note_port->buffer(0)->value_at(0)))
		m_gate_port->buffer(0)->set(0.0f, offset);
}


} // namespace Om

