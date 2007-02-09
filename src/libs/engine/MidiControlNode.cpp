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

#include "MidiControlNode.h"
#include <math.h>
#include "PostProcessor.h"
#include "MidiLearnEvent.h"
#include "InputPort.h"
#include "OutputPort.h"
#include "Plugin.h"
#include "util.h"
#include "midi.h"


namespace Ingen {

	
MidiControlNode::MidiControlNode(const string& path, size_t poly, Patch* parent, SampleRate srate, size_t buffer_size)
: InternalNode(new Plugin(Plugin::Internal, "ingen:control_node"), path, 1, parent, srate, buffer_size),
  _learning(false)
{
	_ports = new Raul::Array<Port*>(7);

	_midi_in_port = new InputPort<MidiMessage>(this, "MIDI_In", 0, 1, DataType::MIDI, _buffer_size);
	_ports->at(0) = _midi_in_port;
	
	_param_port = new InputPort<Sample>(this, "Controller_Number", 1, 1, DataType::FLOAT, 1);
	_ports->at(1) = _param_port;

	_log_port = new InputPort<Sample>(this, "Logarithmic", 2, 1, DataType::FLOAT, 1);
	_ports->at(2) = _log_port;
	
	_min_port = new InputPort<Sample>(this, "Min", 3, 1, DataType::FLOAT, 1);
	_ports->at(3) = _min_port;
	
	_max_port = new InputPort<Sample>(this, "Max", 4, 1, DataType::FLOAT, 1);
	_ports->at(4) = _max_port;
	
	_audio_port = new OutputPort<Sample>(this, "Out_(AR)", 5, 1, DataType::FLOAT, _buffer_size);
	_ports->at(5) = _audio_port;

	_control_port = new OutputPort<Sample>(this, "Out_(CR)", 6, 1, DataType::FLOAT, 1);
	_ports->at(6) = _control_port;
	
	plugin()->plug_label("midi_control_in");
	assert(plugin()->uri() == "ingen:control_node");
	plugin()->name("Ingen Control Node (MIDI)");
}


void
MidiControlNode::process(SampleCount nframes, FrameTime start, FrameTime end)
{
	InternalNode::process(nframes, start, end);

	MidiMessage ev;
	
	for (size_t i=0; i < _midi_in_port->buffer(0)->filled_size(); ++i) {
		ev = _midi_in_port->buffer(0)->value_at(i);

		if ((ev.buffer[0] & 0xF0) == MIDI_CMD_CONTROL)
			control(ev.buffer[1], ev.buffer[2], ev.time);
	}
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

	if (_log_port->buffer(0)->value_at(0) > 0.0f) {
		// haaaaack, stupid negatives and logarithms
		Sample log_offset = 0;
		if (_min_port->buffer(0)->value_at(0) < 0)
			log_offset = fabs(_min_port->buffer(0)->value_at(0));
		const Sample min = log(_min_port->buffer(0)->value_at(0)+1+log_offset);
		const Sample max = log(_max_port->buffer(0)->value_at(0)+1+log_offset);
		scaled_value = expf(nval * (max - min) + min) - 1 - log_offset;
	} else {
		const Sample min = _min_port->buffer(0)->value_at(0);
		const Sample max = _max_port->buffer(0)->value_at(0);
		scaled_value = ((nval) * (max - min)) + min;
	}

	if (control_num == _param_port->buffer(0)->value_at(0)) {
		_control_port->set_value(scaled_value, offset);
		_audio_port->set_value(scaled_value, offset);
	}
}


} // namespace Ingen

