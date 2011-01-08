/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#include <cassert>
#include <cmath>
#include <map>
#include <string>
#include <stdint.h>
#include <boost/optional.hpp>
#include "shared/LV2URIMap.hpp"
#include "raul/log.hpp"
#include "raul/Maid.hpp"
#include "LADSPANode.hpp"
#include "AudioBuffer.hpp"
#include "InputPort.hpp"
#include "OutputPort.hpp"
#include "PluginImpl.hpp"
#include "ProcessContext.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {

using namespace Shared;


/** Partially construct a LADSPANode.
 *
 * Object is not usable until instantiate() is called with success.
 * (It _will_ crash!)
 */
LADSPANode::LADSPANode(PluginImpl*              plugin,
                       const string&            path,
                       bool                     polyphonic,
                       PatchImpl*               parent,
                       const LADSPA_Descriptor* descriptor,
                       SampleRate               srate)
	: NodeImpl(plugin, path, polyphonic, parent, srate)
	, _descriptor(descriptor)
	, _instances(NULL)
	, _prepared_instances(NULL)
{
	assert(_descriptor != NULL);
}


LADSPANode::~LADSPANode()
{
	delete _instances;
}


bool
LADSPANode::prepare_poly(BufferFactory& bufs, uint32_t poly)
{
	if (!_polyphonic)
		poly = 1;

	NodeImpl::prepare_poly(bufs, poly);

	if (_polyphony == poly)
		return true;

	_prepared_instances = new Instances(poly, *_instances, SharedPtr<Instance>());
	for (uint32_t i = _polyphony; i < _prepared_instances->size(); ++i) {
		_prepared_instances->at(i) = SharedPtr<Instance>(new Instance(_descriptor,
				_descriptor->instantiate(_descriptor, _srate)));
		if (!_prepared_instances->at(i)->handle) {
			error << "Failed to instantiate plugin" << endl;
			return false;
		}

		// Initialize the values of new ports
		for (uint32_t j = 0; j < num_ports(); ++j) {
			PortImpl* const port   = _ports->at(j);
			Buffer* const   buffer = port->prepared_buffer(i).get();
			if (buffer) {
				if (port->is_a(PortType::CONTROL)) {
					((AudioBuffer*)buffer)->set_value(port->value().get_float(), 0, 0);
				} else {
					buffer->clear();
				}
			}
		}

		if (_activated && _descriptor->activate)
			_descriptor->activate(_prepared_instances->at(i)->handle);
	}

	return true;
}


bool
LADSPANode::apply_poly(Raul::Maid& maid, uint32_t poly)
{
	if (!_polyphonic)
		poly = 1;

	if (_prepared_instances) {
		maid.push(_instances);
		_instances = _prepared_instances;
		_prepared_instances = NULL;
	}
	assert(poly <= _instances->size());

	return NodeImpl::apply_poly(maid, poly);
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
LADSPANode::instantiate(BufferFactory& bufs)
{
	const LV2URIMap& uris = bufs.uris();
	if (!_ports)
		_ports = new Raul::Array<PortImpl*>(_descriptor->PortCount);

	_instances = new Instances(_polyphony);

	for (uint32_t i = 0; i < _polyphony; ++i) {
		(*_instances)[i] = SharedPtr<Instance>(new Instance(
				_descriptor, _descriptor->instantiate(_descriptor, _srate)));
		if (!instance(i)) {
			error << "Failed to instantiate plugin" << endl;
			return false;
		}
	}

	PortImpl* port = NULL;
	std::map<Symbol, uint32_t> symbols;

	for (uint32_t j=0; j < _descriptor->PortCount; ++j) {
		// Convert name into valid symbol
		Symbol symbol = Symbol::symbolify(_descriptor->PortNames[j]);

		// Ensure symbol is unique
		std::map<Symbol, uint32_t>::iterator existing = symbols.find(symbol);
		uint32_t offset = 2;
		bool type_clash = false;
		while (existing != symbols.end()) {
			const uint32_t e = existing->second;
			if (!type_clash && LADSPA_IS_PORT_CONTROL(_descriptor->PortDescriptors[j])
					&& LADSPA_IS_PORT_AUDIO(_descriptor->PortDescriptors[e])) {
				symbol = string(symbol.c_str()) + "_CR";
				type_clash = true;
			} else if (!type_clash && LADSPA_IS_PORT_AUDIO(_descriptor->PortDescriptors[j])
					&& LADSPA_IS_PORT_CONTROL(_descriptor->PortDescriptors[e])) {
				symbol = string(symbol.c_str()) + "_AR";
				type_clash = true;
			} else {
				std::ostringstream ss;
				ss << symbol << "_" << offset;
				symbol = ss.str();
			}
			existing = symbols.find(symbol);
		}

		symbols.insert(make_pair(symbol, j));

		Path port_path(path().child(symbol));

		PortType type = PortType::AUDIO;

		if (LADSPA_IS_PORT_CONTROL(_descriptor->PortDescriptors[j])) {
			type = PortType::CONTROL;
		} else {
			assert(LADSPA_IS_PORT_AUDIO(_descriptor->PortDescriptors[j]));
		}
		assert (LADSPA_IS_PORT_INPUT(_descriptor->PortDescriptors[j])
			|| LADSPA_IS_PORT_OUTPUT(_descriptor->PortDescriptors[j]));

		boost::optional<Sample> default_val, min, max;
		get_port_limits(j, default_val, min, max);

		const float value = default_val ? default_val.get() : 0.0f;

		if (LADSPA_IS_PORT_INPUT(_descriptor->PortDescriptors[j])) {
			port = new InputPort(bufs, this, symbol, j, _polyphony, type, value);
			_ports->at(j) = port;
		} else if (LADSPA_IS_PORT_OUTPUT(_descriptor->PortDescriptors[j])) {
			port = new OutputPort(bufs, this, symbol, j, _polyphony, type, value);
			_ports->at(j) = port;
		}

		port->set_property(uris.lv2_name, _descriptor->PortNames[j]);

		assert(port);
		assert(_ports->at(j) == port);

		// Work around broke-ass crackhead plugins
		if (default_val && default_val.get() < min.get()) {
			warn << "Broken LADSPA " << _descriptor->UniqueID
					<< ": Port default < minimum.  Minimum adjusted." << endl;
			min = default_val;
		}

		if (default_val && default_val.get() > max.get()) {
			warn << "Broken LADSPA " << _descriptor->UniqueID
					<< ": Maximum adjusted." << endl;
			max = default_val;
		}

		// Set initial/default value
		if (type == PortType::CONTROL)
			port->set_value(value);

		if (port->is_input() && type == PortType::CONTROL) {
			if (min)
				port->set_meta_property(uris.lv2_minimum, min.get());
			if (max)
				port->set_meta_property(uris.lv2_maximum, max.get());
			if (default_val)
				port->set_meta_property(uris.lv2_default, default_val.get());
		}
	}

	return true;
}


void
LADSPANode::activate(BufferFactory& bufs)
{
	NodeImpl::activate(bufs);

	if (_descriptor->activate != NULL)
		for (uint32_t i = 0; i < _polyphony; ++i)
			_descriptor->activate(instance(i));
}


void
LADSPANode::deactivate()
{
	NodeImpl::deactivate();

	for (uint32_t i = 0; i < _polyphony; ++i)
		if (_descriptor->deactivate != NULL)
			_descriptor->deactivate(instance(i));
}


void
LADSPANode::process(ProcessContext& context)
{
	NodeImpl::pre_process(context);

	for (uint32_t i = 0; i < _polyphony; ++i)
		_descriptor->run(instance(i), context.nframes());

	NodeImpl::post_process(context);
}


void
LADSPANode::set_port_buffer(uint32_t voice, uint32_t port_num,
		IntrusivePtr<Buffer> buf, SampleCount offset)
{
	NodeImpl::set_port_buffer(voice, port_num, buf, offset);

	_descriptor->connect_port(instance(voice), port_num,
			buf ? (LADSPA_Data*)buf->port_data(_ports->at(port_num)->buffer_type(), offset) : NULL);
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

