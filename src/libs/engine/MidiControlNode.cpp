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

#include "MidiControlNode.hpp"
#include <math.h>
#include <raul/midi_events.h>
#include "PostProcessor.hpp"
#include "MidiLearnEvent.hpp"
#include "InputPort.hpp"
#include "OutputPort.hpp"
#include "Plugin.hpp"
#include "util.hpp"
#include "AudioBuffer.hpp"

namespace Ingen {

	
MidiControlNode::MidiControlNode(const string& path, size_t poly, Patch* parent, SampleRate srate, size_t buffer_size)
: InternalNode(new Plugin(Plugin::Internal, "ingen:control_node"), path, 1, parent, srate, buffer_size),
  _learning(false)
{
	_ports = new Raul::Array<Port*>(7);

	_midi_in_port = new InputPort(this, "MIDIIn", 0, 1, DataType::MIDI, _buffer_size);
	_ports->at(0) = _midi_in_port;
	
	_param_port = new InputPort(this, "ControllerNumber", 1, 1, DataType::FLOAT, 1);
	_param_port->set_metadata("ingen:minimum", 0.0f);
	_param_port->set_metadata("ingen:maximum", 127.0f);
	_param_port->set_metadata("ingen:default", 0.0f);
	_param_port->set_metadata("ingen:integer", 1);
	_ports->at(1) = _param_port;

	_log_port = new InputPort(this, "Logarithmic", 2, 1, DataType::FLOAT, 1);
	_log_port->set_metadata("ingen:toggled", 1);
	_log_port->set_metadata("ingen:default", 0.0f);
	_ports->at(2) = _log_port;
	
	_min_port = new InputPort(this, "Min", 3, 1, DataType::FLOAT, 1);
	_min_port->set_metadata("ingen:default", 0.0f);
	_ports->at(3) = _min_port;
	
	_max_port = new InputPort(this, "Max", 4, 1, DataType::FLOAT, 1);
	_ports->at(4) = _max_port;
	
	_audio_port = new OutputPort(this, "Out(AR)", 5, 1, DataType::FLOAT, _buffer_size);
	_ports->at(5) = _audio_port;

	_control_port = new OutputPort(this, "Out(CR)", 6, 1, DataType::FLOAT, 1);
	_ports->at(6) = _control_port;
	
	plugin()->plug_label("midi_control_in");
	assert(plugin()->uri() == "ingen:control_node");
	plugin()->name("Ingen Control Node (MIDI)");
}


void
MidiControlNode::process(SampleCount nframes, FrameTime start, FrameTime end)
{
	NodeBase::pre_process(nframes, start, end);
	
	double         timestamp = 0;
	uint32_t       size = 0;
	unsigned char* buffer = NULL;

	MidiBuffer* const midi_in = (MidiBuffer*)_midi_in_port->buffer(0);
	assert(midi_in->this_nframes() == nframes);

	while (midi_in->get_event(&timestamp, &size, &buffer) < nframes) {

		if (size >= 3 && (buffer[0] & 0xF0) == MIDI_CMD_CONTROL)
			control(buffer[1], buffer[2], (SampleCount)timestamp);
		
		midi_in->increment();
	}
	
	NodeBase::post_process(nframes, start, end);
}


void
MidiControlNode::control(uchar control_num, uchar val, SampleCount offset)
{
	assert(offset < _buffer_size);

	Sample scaled_value;
	
	const Sample nval = (val / 127.0f); // normalized [0, 1]
	
	if (_learning) {
		assert(false); // FIXME FIXME FIXME
#if 0
		assert(_learn_event != NULL);
		_param_port->set_value(control_num, offset);
		assert(_param_port->buffer(0)->value_at(0) == control_num);
		_learn_event->set_value(control_num);
		_learn_event->execute(offset);
		//Engine::instance().post_processor()->push(_learn_event);
		//Engine::instance().post_processor()->whip();
		_learning = false;
		_learn_event = NULL;
#endif
	}

	const Sample min_port_val = ((AudioBuffer*)_min_port->buffer(0))->value_at(0);
	const Sample max_port_val = ((AudioBuffer*)_max_port->buffer(0))->value_at(0);
	const Sample log_port_val = ((AudioBuffer*)_log_port->buffer(0))->value_at(0);

	if (log_port_val > 0.0f) {
		// haaaaack, stupid negatives and logarithms
		Sample log_offset = 0;
		if (min_port_val < 0)
			log_offset = fabs(min_port_val);
		const Sample min = log(min_port_val+1+log_offset);
		const Sample max = log(max_port_val+1+log_offset);
		scaled_value = expf(nval * (max - min) + min) - 1 - log_offset;
	} else {
		scaled_value = ((nval) * (max_port_val - min_port_val)) + min_port_val;
	}

	if (control_num == ((AudioBuffer*)_param_port->buffer(0))->value_at(0)) {
		((AudioBuffer*)_control_port->buffer(0))->set(scaled_value, 0);
		((AudioBuffer*)_audio_port->buffer(0))->set(scaled_value, offset);
	}
}


} // namespace Ingen

