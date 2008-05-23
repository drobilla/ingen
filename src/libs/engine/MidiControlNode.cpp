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

#include <math.h>
#include <raul/midi_events.h>
#include "MidiControlNode.hpp"
#include "PostProcessor.hpp"
#include "MidiLearnEvent.hpp"
#include "InputPort.hpp"
#include "OutputPort.hpp"
#include "InternalPlugin.hpp"
#include "AudioBuffer.hpp"
#include "ProcessContext.hpp"
#include "EventBuffer.hpp"
#include "util.hpp"

namespace Ingen {

	
MidiControlNode::MidiControlNode(const string& path,
                                 bool          polyphonic,
                                 PatchImpl*    parent,
                                 SampleRate    srate,
                                 size_t        buffer_size)
	: NodeBase(new InternalPlugin("ingen:control_node", "controller", "MIDI Controller")
			, path, false, parent, srate, buffer_size)
	, _learning(false)
{
	_ports = new Raul::Array<PortImpl*>(7);

	_midi_in_port = new InputPort(this, "input", 0, 1, DataType::EVENT, Atom(), _buffer_size);
	_ports->at(0) = _midi_in_port;
	
	_param_port = new InputPort(this, "controller", 1, 1, DataType::CONTROL, 0.0f, 1);
	_param_port->set_variable("ingen:minimum", 0.0f);
	_param_port->set_variable("ingen:maximum", 127.0f);
	_param_port->set_variable("ingen:integer", 1);
	_ports->at(1) = _param_port;

	_log_port = new InputPort(this, "logarithmic", 2, 1, DataType::CONTROL, 0.0f, 1);
	_log_port->set_variable("ingen:toggled", 1);
	_ports->at(2) = _log_port;
	
	_min_port = new InputPort(this, "minimum", 3, 1, DataType::CONTROL, 0.0f, 1);
	_ports->at(3) = _min_port;
	
	_max_port = new InputPort(this, "maximum", 4, 1, DataType::CONTROL, 1.0f, 1);
	_ports->at(4) = _max_port;
	
	_audio_port = new OutputPort(this, "ar_output", 5, 1, DataType::AUDIO, 0.0f, _buffer_size);
	_ports->at(5) = _audio_port;
}


void
MidiControlNode::process(ProcessContext& context)
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
		// FIXME: type
		if (size >= 3 && (buf[0] & 0xF0) == MIDI_CMD_CONTROL)
			control(buf[1], buf[2], (SampleCount)frames);
		
		midi_in->increment();
	}
	
	NodeBase::post_process(context);
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

	if (control_num == ((AudioBuffer*)_param_port->buffer(0))->value_at(0))
		((AudioBuffer*)_audio_port->buffer(0))->set(scaled_value, offset);
}


} // namespace Ingen

