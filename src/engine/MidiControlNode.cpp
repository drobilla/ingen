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

#include "MidiControlNode.h"
#include <math.h>
#include "Om.h"
#include "OmApp.h"
#include "PostProcessor.h"
#include "MidiLearnEvent.h"
#include "InputPort.h"
#include "OutputPort.h"
#include "PortInfo.h"
#include "Plugin.h"
#include "util.h"
#include "midi.h"


namespace Om {

	
MidiControlNode::MidiControlNode(const string& path, size_t poly, Patch* parent, samplerate srate, size_t buffer_size)
: InternalNode(path, 1, parent, srate, buffer_size),
  m_learning(false)
{
	m_num_ports = 7;
	m_ports.alloc(m_num_ports);

	m_midi_in_port = new InputPort<MidiMessage>(this, "MIDI In", 0, 1,
		new PortInfo("MIDI In", MIDI, INPUT), m_buffer_size);
	m_ports.at(0) = m_midi_in_port;
	
	m_param_port = new InputPort<sample>(this, "Controller Number", 1, 1, 
		new PortInfo("Controller Number", CONTROL, INPUT, INTEGER, 60, 0, 127), 1);
	m_ports.at(1) = m_param_port;
	m_log_port = new InputPort<sample>(this, "Logarithmic", 2, 1,
		new PortInfo("Logarithmic", CONTROL, INPUT, TOGGLE, 0, 0, 1), 1);
	m_ports.at(2) = m_log_port;
	
	m_min_port = new InputPort<sample>(this, "Min", 3, 1, 
		new PortInfo("Min", CONTROL, INPUT, NONE, 0, 0, 65535), 1);
	m_ports.at(3) = m_min_port;
	
	m_max_port = new InputPort<sample>(this, "Max", 4, 1, 
		new PortInfo("Max", CONTROL, INPUT, NONE, 1, 0, 65535), 1);
	m_ports.at(4) = m_max_port;
	
	m_audio_port = new OutputPort<sample>(this, "Out (AR)", 5, 1, 
		new PortInfo("Out (AR)", AUDIO, OUTPUT, 0, 0, 1), m_buffer_size);
	m_ports.at(5) = m_audio_port;

	m_control_port = new OutputPort<sample>(this, "Out (CR)", 6, 1, 
		new PortInfo("Out (CR)", CONTROL, OUTPUT, 0, 0, 1), 1);
	m_ports.at(6) = m_control_port;
	
	m_plugin.type(Plugin::Internal);
	m_plugin.plug_label("midi_control_in");
	m_plugin.name("Om Control Node (MIDI)");
}


void
MidiControlNode::run(size_t nframes)
{
	InternalNode::run(nframes);

	MidiMessage ev;
	
	for (size_t i=0; i < m_midi_in_port->buffer(0)->filled_size(); ++i) {
		ev = m_midi_in_port->buffer(0)->value_at(i);

		if ((ev.buffer[0] & 0xF0) == MIDI_CMD_CONTROL)
			control(ev.buffer[1], ev.buffer[2], ev.time);
	}
}


void
MidiControlNode::control(uchar control_num, uchar val, samplecount offset)
{
	assert(offset < m_buffer_size);

	sample scaled_value;
	
	const sample nval = (val / 127.0f); // normalized [0, 1]
	
	if (m_learning) {
		assert(m_learn_event != NULL);
		m_param_port->set_value(control_num, offset);
		assert(m_param_port->buffer(0)->value_at(0) == control_num);
		m_learn_event->set_value(control_num);
		m_learn_event->execute(offset);
		om->post_processor()->push(m_learn_event);
		om->post_processor()->signal();
		m_learning = false;
		m_learn_event = NULL;
	}

	if (m_log_port->buffer(0)->value_at(0) > 0.0f) {
		// haaaaack, stupid negatives and logarithms
		sample log_offset = 0;
		if (m_min_port->buffer(0)->value_at(0) < 0)
			log_offset = fabs(m_min_port->buffer(0)->value_at(0));
		const sample min = log(m_min_port->buffer(0)->value_at(0)+1+log_offset);
		const sample max = log(m_max_port->buffer(0)->value_at(0)+1+log_offset);
		scaled_value = expf(nval * (max - min) + min) - 1 - log_offset;
	} else {
		const sample min = m_min_port->buffer(0)->value_at(0);
		const sample max = m_max_port->buffer(0)->value_at(0);
		scaled_value = ((nval) * (max - min)) + min;
	}

	if (control_num == m_param_port->buffer(0)->value_at(0)) {
		m_control_port->set_value(scaled_value, offset);
		m_audio_port->set_value(scaled_value, offset);
	}
}


} // namespace Om

