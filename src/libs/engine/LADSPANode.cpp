/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */


#include "LADSPANode.h"
#include <iostream>
#include <cassert>
#include "float.h"
#include <stdint.h>
#include <cmath>
#include "InputPort.h"
#include "OutputPort.h"
#include "Plugin.h"

namespace Om {


/** Partially construct a LADSPANode.
 *
 * Object is not usable until instantiate() is called with success.
 * (It _will_ crash!)
 */
LADSPANode::LADSPANode(const Plugin* plugin, const string& path, size_t poly, Patch* parent, const LADSPA_Descriptor* descriptor, samplerate srate, size_t buffer_size)
: NodeBase(plugin, path, poly, parent, srate, buffer_size),
  _descriptor(descriptor),
  _instances(NULL)
{
	assert(_descriptor != NULL);
}


/** Instantiate self from LADSPA plugin descriptor.
 *
 * Implemented as a seperate function (rather than in the constructor) to
 * allow graceful error-catching of broken plugins.
 * 
 * Returns whether or not plugin was successfully instantiated.  If return
 * value is false, this object may not be used.
 */
bool
LADSPANode::instantiate()
{
	// Note that a DSSI plugin might tack more on to the end of this
	if (!_ports)
		_ports = new Array<Port*>(_descriptor->PortCount);
	
	_instances = new LADSPA_Handle[_poly];
	
	size_t port_buffer_size = 0;
	
	for (size_t i=0; i < _poly; ++i) {
		_instances[i] = _descriptor->instantiate(_descriptor, _srate);
		if (_instances[i] == NULL) {
			cerr << "Failed to instantiate plugin!" << endl;
			return false;
		}
	}
	
	string port_name;
	string port_path;
	
	Port* port = NULL;
	
	for (size_t j=0; j < _descriptor->PortCount; ++j) {
		port_name = _descriptor->PortNames[j];
		string::size_type slash_index;
		
		// Name mangling, to guarantee port names are unique
		for (size_t k=0; k < _descriptor->PortCount; ++k) {
			assert(_descriptor->PortNames[k] != NULL);
			if (k != j && port_name == _descriptor->PortNames[k]) { // clash
				if (LADSPA_IS_PORT_CONTROL(_descriptor->PortDescriptors[j]))
					port_name += " (CR)";
				else
					port_name += " (AR)";
			}
			// Replace all slashes with "-" (so they don't screw up paths)
			while ((slash_index = port_name.find("/")) != string::npos)
				port_name[slash_index] = '-';
		}

		port_path = path() + "/" + port_name;

		if (LADSPA_IS_PORT_CONTROL(_descriptor->PortDescriptors[j]))
			port_buffer_size = 1;
		else if (LADSPA_IS_PORT_AUDIO(_descriptor->PortDescriptors[j]))
			port_buffer_size = _buffer_size;
		
		assert (LADSPA_IS_PORT_INPUT(_descriptor->PortDescriptors[j])
			|| LADSPA_IS_PORT_OUTPUT(_descriptor->PortDescriptors[j]));

		if (LADSPA_IS_PORT_INPUT(_descriptor->PortDescriptors[j])) {
			port = new InputPort<sample>(this, port_name, j, _poly, DataType::FLOAT, port_buffer_size);
			_ports->at(j) = port;
		} else if (LADSPA_IS_PORT_OUTPUT(_descriptor->PortDescriptors[j])) {
			port = new OutputPort<sample>(this, port_name, j, _poly, DataType::FLOAT, port_buffer_size);
			_ports->at(j) = port;
		}

		assert(_ports->at(j) != NULL);
		sample default_val = default_port_value(j);

		// Set default control val
		if (port->buffer_size() == 1) {
			((TypedPort<sample>*)port)->set_value(default_val, 0);
		} else {
			((TypedPort<sample>*)port)->set_value(0.0f, 0);
		}
	}

	return true;
}


LADSPANode::~LADSPANode()
{
	for (size_t i=0; i < _poly; ++i)
		_descriptor->cleanup(_instances[i]);

	delete[] _instances;
}


void
LADSPANode::activate()
{
	NodeBase::activate();

	TypedPort<sample>* port = NULL;
	
	for (size_t i=0; i < _poly; ++i) {
		for (unsigned long j=0; j < _descriptor->PortCount; ++j) {
			port = static_cast<TypedPort<sample>*>(_ports->at(j));
			set_port_buffer(i, j, ((TypedPort<sample>*)_ports->at(j))->buffer(i)->data());
			/*	if (port->type() == DataType::FLOAT && port->buffer_size() == 1)
					port->set_value(0.0f, 0); // FIXME
				else if (port->type() == DataType::FLOAT && port->buffer_size() > 1)
					port->set_value(0.0f, 0);*/
		}
		if (_descriptor->activate != NULL)
			_descriptor->activate(_instances[i]);
	}
}


void
LADSPANode::deactivate()
{
	NodeBase::deactivate();
	
	for (size_t i=0; i < _poly; ++i)
		if (_descriptor->deactivate != NULL)
			_descriptor->deactivate(_instances[i]);
}


void
LADSPANode::process(samplecount nframes)
{
	NodeBase::process(nframes); // mixes down input ports
	for (size_t i=0; i < _poly; ++i) 
		_descriptor->run(_instances[i], nframes);
}


void
LADSPANode::set_port_buffer(size_t voice, size_t port_num, void* buf)
{
	assert(voice < _poly);
	
	// Could be a MIDI port after this
	if (port_num < _descriptor->PortCount)
		_descriptor->connect_port(_instances[voice], port_num, (sample*)buf);
}

#if 0
// Based on code stolen from jack-rack
void
LADSPANode::get_port_vals(ulong port_index, PortInfo* info)
{
	LADSPA_Data upper = 0.0f;
	LADSPA_Data lower = 0.0f;
	LADSPA_Data normal = 0.0f;
	LADSPA_PortRangeHintDescriptor hint_descriptor = _descriptor->PortRangeHints[port_index].HintDescriptor;

	/* set upper and lower, possibly adjusted to the sample rate */
	if (LADSPA_IS_HINT_SAMPLE_RATE(hint_descriptor)) {
		upper = _descriptor->PortRangeHints[port_index].UpperBound * _srate;
		lower = _descriptor->PortRangeHints[port_index].LowerBound * _srate;
	} else {
		upper = _descriptor->PortRangeHints[port_index].UpperBound;
		lower = _descriptor->PortRangeHints[port_index].LowerBound;
	}

	if (LADSPA_IS_HINT_LOGARITHMIC(hint_descriptor)) {
		/* FLT_EPSILON is defined as the different between 1.0 and the minimum
		 * float greater than 1.0.  So, if lower is < FLT_EPSILON, it will be 1.0
		 * and the logarithmic control will have a base of 1 and thus not change
		 */
		if (lower < FLT_EPSILON) lower = FLT_EPSILON;
	}


	if (LADSPA_IS_HINT_HAS_DEFAULT(hint_descriptor)) {

		if (LADSPA_IS_HINT_DEFAULT_MINIMUM(hint_descriptor)) {
			normal = lower;
		} else if (LADSPA_IS_HINT_DEFAULT_LOW(hint_descriptor)) {
			if (LADSPA_IS_HINT_LOGARITHMIC(hint_descriptor)) {
				normal = exp(log(lower) * 0.75 + log(upper) * 0.25);
			} else {
				normal = lower * 0.75 + upper * 0.25;
			}
		} else if (LADSPA_IS_HINT_DEFAULT_MIDDLE(hint_descriptor)) {
			if (LADSPA_IS_HINT_LOGARITHMIC(hint_descriptor)) {
				normal = exp(log(lower) * 0.5 + log(upper) * 0.5);
			} else {
				normal = lower * 0.5 + upper * 0.5;
			}
		} else if (LADSPA_IS_HINT_DEFAULT_HIGH(hint_descriptor)) {
			if (LADSPA_IS_HINT_LOGARITHMIC(hint_descriptor)) {
				normal = exp(log(lower) * 0.25 + log(upper) * 0.75);
			} else {
				normal = lower * 0.25 + upper * 0.75;
			}
		} else if (LADSPA_IS_HINT_DEFAULT_MAXIMUM(hint_descriptor)) {
			normal = upper;
		} else if (LADSPA_IS_HINT_DEFAULT_0(hint_descriptor)) {
			normal = 0.0;
		} else if (LADSPA_IS_HINT_DEFAULT_1(hint_descriptor)) {
			normal = 1.0;
		} else if (LADSPA_IS_HINT_DEFAULT_100(hint_descriptor)) {
			normal = 100.0;
		} else if (LADSPA_IS_HINT_DEFAULT_440(hint_descriptor)) {
			normal = 440.0;
		}
	} else {  // No default hint
		if (LADSPA_IS_HINT_BOUNDED_BELOW(hint_descriptor)) {
			normal = lower;
		} else if (LADSPA_IS_HINT_BOUNDED_ABOVE(hint_descriptor)) {
			normal = upper;
		}
	}

	info->min_val(lower);
	info->default_val(normal);
	info->max_val(upper);
}
#endif


sample
LADSPANode::default_port_value(ulong port_index)
{
	LADSPA_Data normal = 0.0f;
	LADSPA_Data upper = 0.0f;
	LADSPA_Data lower = 0.0f;
	LADSPA_PortRangeHintDescriptor hint_descriptor = _descriptor->PortRangeHints[port_index].HintDescriptor;

	/* set upper and lower, possibly adjusted to the sample rate */
	if (LADSPA_IS_HINT_SAMPLE_RATE(hint_descriptor)) {
		upper = _descriptor->PortRangeHints[port_index].UpperBound * _srate;
		lower = _descriptor->PortRangeHints[port_index].LowerBound * _srate;
	} else {
		upper = _descriptor->PortRangeHints[port_index].UpperBound;
		lower = _descriptor->PortRangeHints[port_index].LowerBound;
	}

	if (LADSPA_IS_HINT_LOGARITHMIC(hint_descriptor)) {
		/* FLT_EPSILON is defined as the different between 1.0 and the minimum
		 * float greater than 1.0.  So, if lower is < FLT_EPSILON, it will be 1.0
		 * and the logarithmic control will have a base of 1 and thus not change
		 */
		if (lower < FLT_EPSILON) lower = FLT_EPSILON;
	}


	if (LADSPA_IS_HINT_HAS_DEFAULT(hint_descriptor)) {

		if (LADSPA_IS_HINT_DEFAULT_MINIMUM(hint_descriptor)) {
			normal = lower;
		} else if (LADSPA_IS_HINT_DEFAULT_LOW(hint_descriptor)) {
			if (LADSPA_IS_HINT_LOGARITHMIC(hint_descriptor)) {
				normal = exp(log(lower) * 0.75 + log(upper) * 0.25);
			} else {
				normal = lower * 0.75 + upper * 0.25;
			}
		} else if (LADSPA_IS_HINT_DEFAULT_MIDDLE(hint_descriptor)) {
			if (LADSPA_IS_HINT_LOGARITHMIC(hint_descriptor)) {
				normal = exp(log(lower) * 0.5 + log(upper) * 0.5);
			} else {
				normal = lower * 0.5 + upper * 0.5;
			}
		} else if (LADSPA_IS_HINT_DEFAULT_HIGH(hint_descriptor)) {
			if (LADSPA_IS_HINT_LOGARITHMIC(hint_descriptor)) {
				normal = exp(log(lower) * 0.25 + log(upper) * 0.75);
			} else {
				normal = lower * 0.25 + upper * 0.75;
			}
		} else if (LADSPA_IS_HINT_DEFAULT_MAXIMUM(hint_descriptor)) {
			normal = upper;
		} else if (LADSPA_IS_HINT_DEFAULT_0(hint_descriptor)) {
			normal = 0.0;
		} else if (LADSPA_IS_HINT_DEFAULT_1(hint_descriptor)) {
			normal = 1.0;
		} else if (LADSPA_IS_HINT_DEFAULT_100(hint_descriptor)) {
			normal = 100.0;
		} else if (LADSPA_IS_HINT_DEFAULT_440(hint_descriptor)) {
			normal = 440.0;
		}
	} else {  // No default hint
		if (LADSPA_IS_HINT_BOUNDED_BELOW(hint_descriptor)) {
			normal = lower;
		} else if (LADSPA_IS_HINT_BOUNDED_ABOVE(hint_descriptor)) {
			normal = upper;
		}
	}

	cerr << path() << " Port " << port_index << " LADSPA Default value: " << normal << endl;
	// FIXME: set min/max as metadata
	return normal;
}


} // namespace Om

