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

#include "LV2Node.h"
#include <iostream>
#include <cassert>
#include "float.h"
#include <stdint.h>
#include <cmath>
#include "InputPort.h"
#include "OutputPort.h"
#include "Plugin.h"
#include "AudioBuffer.h"

namespace Ingen {


/** Partially construct a LV2Node.
 *
 * Object is not usable until instantiate() is called with success.
 * (It _will_ crash!)
 */
LV2Node::LV2Node(const Plugin*      plugin,
                 const string&      name,
                 size_t             poly,
                 Patch*             parent,
                 SampleRate         srate,
                 size_t             buffer_size)
: NodeBase(plugin, name, poly, parent, srate, buffer_size),
  _lv2_plugin(plugin->slv2_plugin()),
  _instances(NULL)
{
	assert(_lv2_plugin);
}


/** Instantiate self from LV2 plugin descriptor.
 *
 * Implemented as a seperate function (rather than in the constructor) to
 * allow graceful error-catching of broken plugins.
 * 
 * Returns whether or not plugin was successfully instantiated.  If return
 * value is false, this object may not be used.
 */
bool
LV2Node::instantiate()
{
	size_t num_ports = slv2_plugin_get_num_ports(_lv2_plugin);
	assert(num_ports > 0);

	_ports = new Raul::Array<Port*>(num_ports);
	
	_instances = new SLV2Instance[_poly];
	
	size_t port_buffer_size = 0;
	
	for (size_t i=0; i < _poly; ++i) {
		_instances[i] = slv2_plugin_instantiate(_lv2_plugin, _srate, NULL);
		if (_instances[i] == NULL) {
			cerr << "Failed to instantiate plugin!" << endl;
			return false;
		}
	}
	
	string port_name;
	string port_path;
	
	Port* port = NULL;
	
	for (size_t j=0; j < num_ports; ++j) {
		SLV2PortID id = slv2_port_by_index(j);

		// LV2 shortnames are guaranteed to be unique, valid C identifiers
		port_name = (char*)slv2_port_get_symbol(_lv2_plugin, id);
	
		assert(port_name.find("/") == string::npos);

		port_path = path() + "/" + port_name;
		
		SLV2PortClass port_class = slv2_port_get_class(_lv2_plugin, id);

		if (port_class == SLV2_AUDIO_INPUT || port_class == SLV2_AUDIO_OUTPUT)
			port_buffer_size = _buffer_size;
		else
			port_buffer_size = 1;
		
		if (port_class == SLV2_CONTROL_INPUT || port_class == SLV2_AUDIO_INPUT) {
			port = new InputPort(this, port_name, j, _poly, DataType::FLOAT, port_buffer_size);
			_ports->at(j) = port;
		} else if (port_class == SLV2_CONTROL_OUTPUT || port_class == SLV2_AUDIO_OUTPUT) {
			port = new OutputPort(this, port_name, j, _poly, DataType::FLOAT, port_buffer_size);
			_ports->at(j) = port;
		} else if (port_class == SLV2_MIDI_INPUT) {
			port = new InputPort(this, port_name, j, _poly, DataType::MIDI, port_buffer_size);
			_ports->at(j) = port;
		} else if (port_class == SLV2_MIDI_OUTPUT) {
			port = new OutputPort(this, port_name, j, _poly, DataType::MIDI, port_buffer_size);
			_ports->at(j) = port;
		}

		assert(_ports->at(j) != NULL);

		//PortInfo* pi = port->port_info();
		//get_port_vals(j, pi);

		// Set default control val
		/*if (pi->is_control())
			((TypedPort<Sample>*)port)->set_value(pi->default_val(), 0);
		else
			((TypedPort<Sample>*)port)->set_value(0.0f, 0);*/
	}
	return true;
}


LV2Node::~LV2Node()
{
	for (size_t i=0; i < _poly; ++i)
		slv2_instance_free(_instances[i]);

	delete[] _instances;
}


void
LV2Node::activate()
{
	NodeBase::activate();

	for (size_t i=0; i < _poly; ++i) {
		for (unsigned long j=0; j < num_ports(); ++j) {
			Port* const port = _ports->at(j);
			set_port_buffer(i, j, port->buffer(i));
			if (port->type() == DataType::FLOAT && port->buffer_size() == 1) {
				cerr << "FIXME: LV2 default value\n";
				((AudioBuffer*)port->buffer(i))->set(0.0f, 0); // FIXME
			} else if (port->type() == DataType::FLOAT && port->buffer_size() > 1) {
				((AudioBuffer*)port->buffer(i))->set(0.0f, 0);
			}
		}
		slv2_instance_activate(_instances[i]);
	}
}


void
LV2Node::deactivate()
{
	NodeBase::deactivate();
	
	for (size_t i=0; i < _poly; ++i)
		slv2_instance_deactivate(_instances[i]);
}


void
LV2Node::process(SampleCount nframes, FrameTime start, FrameTime end)
{
	NodeBase::pre_process(nframes, start, end);

	for (size_t i=0; i < _poly; ++i) 
		slv2_instance_run(_instances[i], nframes);
	
	NodeBase::post_process(nframes, start, end);
}


void
LV2Node::set_port_buffer(size_t voice, size_t port_num, Buffer* buf)
{
	assert(voice < _poly);
	
	if (buf->type() == DataType::FLOAT)
		slv2_instance_connect_port(_instances[voice], port_num, ((AudioBuffer*)buf)->data());
	else if (buf->type() == DataType::MIDI)
		slv2_instance_connect_port(_instances[voice], port_num, ((MidiBuffer*)buf)->data());
}


#if 0
// Based on code stolen from jack-rack
void
LV2Node::get_port_vals(ulong port_index, PortInfo* info)
{
	LV2_Data upper = 0.0f;
	LV2_Data lower = 0.0f;
	LV2_Data normal = 0.0f;
	LV2_PortRangeHintDescriptor hint_descriptor = _descriptor->PortRangeHints[port_index].HintDescriptor;

	/* set upper and lower, possibly adjusted to the sample rate */
	if (LV2_IS_HINT_SAMPLE_RATE(hint_descriptor)) {
		upper = _descriptor->PortRangeHints[port_index].UpperBound * _srate;
		lower = _descriptor->PortRangeHints[port_index].LowerBound * _srate;
	} else {
		upper = _descriptor->PortRangeHints[port_index].UpperBound;
		lower = _descriptor->PortRangeHints[port_index].LowerBound;
	}

	if (LV2_IS_HINT_LOGARITHMIC(hint_descriptor)) {
		/* FLT_EPSILON is defined as the different between 1.0 and the minimum
		 * float greater than 1.0.  So, if lower is < FLT_EPSILON, it will be 1.0
		 * and the logarithmic control will have a base of 1 and thus not change
		 */
		if (lower < FLT_EPSILON) lower = FLT_EPSILON;
	}


	if (LV2_IS_HINT_HAS_DEFAULT(hint_descriptor)) {

		if (LV2_IS_HINT_DEFAULT_MINIMUM(hint_descriptor)) {
			normal = lower;
		} else if (LV2_IS_HINT_DEFAULT_LOW(hint_descriptor)) {
			if (LV2_IS_HINT_LOGARITHMIC(hint_descriptor)) {
				normal = exp(log(lower) * 0.75 + log(upper) * 0.25);
			} else {
				normal = lower * 0.75 + upper * 0.25;
			}
		} else if (LV2_IS_HINT_DEFAULT_MIDDLE(hint_descriptor)) {
			if (LV2_IS_HINT_LOGARITHMIC(hint_descriptor)) {
				normal = exp(log(lower) * 0.5 + log(upper) * 0.5);
			} else {
				normal = lower * 0.5 + upper * 0.5;
			}
		} else if (LV2_IS_HINT_DEFAULT_HIGH(hint_descriptor)) {
			if (LV2_IS_HINT_LOGARITHMIC(hint_descriptor)) {
				normal = exp(log(lower) * 0.25 + log(upper) * 0.75);
			} else {
				normal = lower * 0.25 + upper * 0.75;
			}
		} else if (LV2_IS_HINT_DEFAULT_MAXIMUM(hint_descriptor)) {
			normal = upper;
		} else if (LV2_IS_HINT_DEFAULT_0(hint_descriptor)) {
			normal = 0.0;
		} else if (LV2_IS_HINT_DEFAULT_1(hint_descriptor)) {
			normal = 1.0;
		} else if (LV2_IS_HINT_DEFAULT_100(hint_descriptor)) {
			normal = 100.0;
		} else if (LV2_IS_HINT_DEFAULT_440(hint_descriptor)) {
			normal = 440.0;
		}
	} else {  // No default hint
		if (LV2_IS_HINT_BOUNDED_BELOW(hint_descriptor)) {
			normal = lower;
		} else if (LV2_IS_HINT_BOUNDED_ABOVE(hint_descriptor)) {
			normal = upper;
		}
	}

	info->min_val(lower);
	info->default_val(normal);
	info->max_val(upper);
}
#endif


} // namespace Ingen

