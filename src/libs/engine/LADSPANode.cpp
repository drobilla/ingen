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

#include <iostream>
#include <cassert>
#include <cmath>
#include <stdint.h>
#include <raul/Maid.hpp>
#include <boost/optional.hpp>
#include "LADSPANode.hpp"
#include "AudioBuffer.hpp"
#include "InputPort.hpp"
#include "OutputPort.hpp"
#include "PluginImpl.hpp"
#include "ProcessContext.hpp"

using namespace std;

namespace Ingen {


/** Partially construct a LADSPANode.
 *
 * Object is not usable until instantiate() is called with success.
 * (It _will_ crash!)
 */
LADSPANode::LADSPANode(PluginImpl* plugin, const string& path, bool polyphonic, PatchImpl* parent, const LADSPA_Descriptor* descriptor, SampleRate srate, size_t buffer_size)
	: NodeBase(plugin, path, polyphonic, parent, srate, buffer_size)
	, _descriptor(descriptor)
	, _instances(NULL)
	, _prepared_instances(NULL)
{
	assert(_descriptor != NULL);
}


LADSPANode::~LADSPANode()
{
	for (uint32_t i=0; i < _polyphony; ++i)
		_descriptor->cleanup((*_instances)[i]);

	delete _instances;
}


bool
LADSPANode::prepare_poly(uint32_t poly)
{
	NodeBase::prepare_poly(poly);

	if ( (!_polyphonic)
			|| (_prepared_instances && poly <= _prepared_instances->size()) ) {
		return true;
	}
	
	_prepared_instances = new Raul::Array<LADSPA_Handle>(poly, *_instances);
	for (uint32_t i = _polyphony; i < _prepared_instances->size(); ++i) {
		_prepared_instances->at(i) = _descriptor->instantiate(_descriptor, _srate);
		if ((*_prepared_instances)[i] == NULL) {
			cerr << "Failed to instantiate plugin!" << endl;
			return false;
		}

		if (_activated)
			_descriptor->activate((*_prepared_instances)[i]);
	}
	
	return true;
}


bool
LADSPANode::apply_poly(Raul::Maid& maid, uint32_t poly)
{
	if (!_polyphonic)
		return true;

	if (_prepared_instances) {
		assert(poly <= _prepared_instances->size());
		maid.push(_instances);
		_instances = _prepared_instances;
		_prepared_instances = NULL;
	}
		
	assert(poly <= _instances->size());
	_polyphony = poly;
	
	NodeBase::apply_poly(maid, poly);

	return true;
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
	if (!_ports)
		_ports = new Raul::Array<PortImpl*>(_descriptor->PortCount);
	
	_instances = new Raul::Array<LADSPA_Handle>(_polyphony, NULL);
	
	size_t port_buffer_size = 0;
	
	for (uint32_t i=0; i < _polyphony; ++i) {
		(*_instances)[i] = _descriptor->instantiate(_descriptor, _srate);
		if ((*_instances)[i] == NULL) {
			cerr << "Failed to instantiate plugin!" << endl;
			return false;
		}
	}
	
	string port_name;
	string port_path;
	
	PortImpl* port = NULL;
	
	for (size_t j=0; j < _descriptor->PortCount; ++j) {
		port_name = Path::nameify(_descriptor->PortNames[j]);

		string::size_type slash_index;
		
		// Name mangling, to guarantee port names are unique
		for (size_t k=0; k < _descriptor->PortCount; ++k) {
			assert(_descriptor->PortNames[k] != NULL);
			if (k != j && port_name == _descriptor->PortNames[k]) { // clash
				if (LADSPA_IS_PORT_CONTROL(_descriptor->PortDescriptors[j]))
					port_name += "(CR)";
				else
					port_name += "(AR)";
			}
			// Replace all slashes with "-" (so they don't screw up paths)
			while ((slash_index = port_name.find("/")) != string::npos)
				port_name[slash_index] = '-';
		}
		
		/*if (_descriptor->PortNames[j] != port_name)
			cerr << "NOTICE: Translated LADSPA port name: " <<
				_descriptor->PortNames[j] << " -> " << port_name << endl;*/

		port_path = path() + "/" + port_name;

		DataType type = DataType::AUDIO;
		port_buffer_size = _buffer_size;

		if (LADSPA_IS_PORT_CONTROL(_descriptor->PortDescriptors[j])) {
			type = DataType::CONTROL;
			port_buffer_size = 1;
		} else {
			assert(LADSPA_IS_PORT_AUDIO(_descriptor->PortDescriptors[j]));
		}
		assert (LADSPA_IS_PORT_INPUT(_descriptor->PortDescriptors[j])
			|| LADSPA_IS_PORT_OUTPUT(_descriptor->PortDescriptors[j]));
		
		boost::optional<Sample> default_val, min, max;
		get_port_limits(j, default_val, min, max);
	
		const float value = default_val ? default_val.get() : 0.0f;

		if (LADSPA_IS_PORT_INPUT(_descriptor->PortDescriptors[j])) {
			port = new InputPort(this, port_name, j, _polyphony, type, value, port_buffer_size);
			_ports->at(j) = port;
		} else if (LADSPA_IS_PORT_OUTPUT(_descriptor->PortDescriptors[j])) {
			port = new OutputPort(this, port_name, j, _polyphony, type, value, port_buffer_size);
			_ports->at(j) = port;
		}

		assert(port);
		assert(_ports->at(j) == port);

		// Work around broke-ass crackhead plugins
		if (default_val && default_val.get() < min.get()) {
			cerr << "WARNING: Broken LADSPA " << _descriptor->UniqueID
					<< ": Port default < minimum.  Minimum adjusted." << endl;
			min = default_val;
		}
		
		if (default_val && default_val.get() > max.get()) {
			cerr << "WARNING: Broken LADSPA " << _descriptor->UniqueID
					<< ": Maximum adjusted." << endl;
			max = default_val;
		}
		
		// Set initial/default value
		if (port->buffer_size() == 1) {
			for (uint32_t i=0; i < _polyphony; ++i)
				((AudioBuffer*)port->buffer(i))->set(value, 0);
		}

		if (port->is_input() && port->buffer_size() == 1) {
			if (min)
				port->set_variable("ingen:minimum", min.get());
			if (max)
				port->set_variable("ingen:maximum", max.get());
			if (default_val)
				port->set_variable("ingen:default", default_val.get());
		}
	}

	return true;
}


void
LADSPANode::activate()
{
	NodeBase::activate();

	for (uint32_t i=0; i < _polyphony; ++i) {
		for (unsigned long j=0; j < _descriptor->PortCount; ++j) {
			set_port_buffer(i, j, _ports->at(j)->buffer(i));
			/*	if (port->type() == DataType::FLOAT && port->buffer_size() == 1)
					port->set_value(0.0f, 0); // FIXME
				else if (port->type() == DataType::FLOAT && port->buffer_size() > 1)
					port->set_value(0.0f, 0);*/
		}
		if (_descriptor->activate != NULL)
			_descriptor->activate((*_instances)[i]);
	}
}


void
LADSPANode::deactivate()
{
	NodeBase::deactivate();
	
	for (uint32_t i=0; i < _polyphony; ++i)
		if (_descriptor->deactivate != NULL)
			_descriptor->deactivate((*_instances)[i]);
}


void
LADSPANode::process(ProcessContext& context)
{
	NodeBase::pre_process(context);

	for (uint32_t i=0; i < _polyphony; ++i) 
		_descriptor->run((*_instances)[i], context.nframes());
	
	NodeBase::post_process(context);
}


void
LADSPANode::set_port_buffer(uint32_t voice, uint32_t port_num, Buffer* buf)
{
	assert(voice < _polyphony);
	
	AudioBuffer* audio_buffer = dynamic_cast<AudioBuffer*>(buf);
	assert(audio_buffer);

	if (port_num < _descriptor->PortCount)
		_descriptor->connect_port((*_instances)[voice], port_num,
			audio_buffer->data());
}


void
LADSPANode::get_port_limits(unsigned long            port_index,
                            boost::optional<Sample>& normal,
                            boost::optional<Sample>& lower,
                            boost::optional<Sample>& upper)
{
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
		if (lower.get() < FLT_EPSILON) lower = FLT_EPSILON;
	}


	if (LADSPA_IS_HINT_HAS_DEFAULT(hint_descriptor)) {

		if (LADSPA_IS_HINT_DEFAULT_MINIMUM(hint_descriptor)) {
			normal = lower;
		} else if (LADSPA_IS_HINT_DEFAULT_LOW(hint_descriptor)) {
			assert(lower);
			assert(upper);
			if (LADSPA_IS_HINT_LOGARITHMIC(hint_descriptor)) {
				normal = exp(log(lower.get()) * 0.75 + log(upper.get()) * 0.25);
			} else {
				normal = lower.get() * 0.75 + upper.get() * 0.25;
			}
		} else if (LADSPA_IS_HINT_DEFAULT_MIDDLE(hint_descriptor)) {
			assert(lower);
			assert(upper);
			if (LADSPA_IS_HINT_LOGARITHMIC(hint_descriptor)) {
				normal = exp(log(lower.get()) * 0.5 + log(upper.get()) * 0.5);
			} else {
				normal = lower.get() * 0.5 + upper.get() * 0.5;
			}
		} else if (LADSPA_IS_HINT_DEFAULT_HIGH(hint_descriptor)) {
			assert(lower);
			assert(upper);
			if (LADSPA_IS_HINT_LOGARITHMIC(hint_descriptor)) {
				normal = exp(log(lower.get()) * 0.25 + log(upper.get()) * 0.75);
			} else {
				normal = lower.get() * 0.25 + upper.get() * 0.75;
			}
		} else if (LADSPA_IS_HINT_DEFAULT_MAXIMUM(hint_descriptor)) {
			assert(upper);
			normal = upper.get();
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
			assert(lower);
			normal = lower.get();
		} else if (LADSPA_IS_HINT_BOUNDED_ABOVE(hint_descriptor)) {
			assert(upper);
			normal = upper.get();
		}
	}
}


} // namespace Ingen

