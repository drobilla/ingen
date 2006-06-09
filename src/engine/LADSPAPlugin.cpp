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


#include "LADSPAPlugin.h"
#include <iostream>
#include <cassert>
#include "float.h"
#include <stdint.h>
#include <cmath>
#include "InputPort.h"
#include "OutputPort.h"
#include "PortInfo.h"
#include "Plugin.h"

namespace Om {


/** Partially construct a LADSPAPlugin.
 *
 * Object is not usable until instantiate() is called with success.
 * (It _will_ crash!)
 */
LADSPAPlugin::LADSPAPlugin(const string& path, size_t poly, Patch* parent, const LADSPA_Descriptor* descriptor, samplerate srate, size_t buffer_size)
: NodeBase(path, poly, parent, srate, buffer_size),
  m_descriptor(descriptor),
  m_instances(NULL)
{
	assert(m_descriptor != NULL);
	
	// Note that this may be changed by an overriding DSSIPlugin
	// ie do not assume m_ports is all LADSPA plugin ports
	m_num_ports = m_descriptor->PortCount;	
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
LADSPAPlugin::instantiate()
{
	m_ports.alloc(m_num_ports);
	
	m_instances = new LADSPA_Handle[m_poly];
	
	size_t port_buffer_size = 0;
	
	for (size_t i=0; i < m_poly; ++i) {
		m_instances[i] = m_descriptor->instantiate(m_descriptor, m_srate);
		if (m_instances[i] == NULL) {
			cerr << "Failed to instantiate plugin!" << endl;
			return false;
		}
	}
	
	string port_name;
	string port_path;
	
	Port* port = NULL;
	
	for (size_t j=0; j < m_descriptor->PortCount; ++j) {
		port_name = m_descriptor->PortNames[j];
		string::size_type slash_index;
		
		// Name mangling, to guarantee port names are unique
		for (size_t k=0; k < m_descriptor->PortCount; ++k) {
			assert(m_descriptor->PortNames[k] != NULL);
			if (k != j && port_name == m_descriptor->PortNames[k]) { // clash
				if (LADSPA_IS_PORT_CONTROL(m_descriptor->PortDescriptors[j]))
					port_name += " (CR)";
				else
					port_name += " (AR)";
			}
			// Replace all slashes with "-" (so they don't screw up paths)
			while ((slash_index = port_name.find("/")) != string::npos)
				port_name[slash_index] = '-';
		}

		port_path = path() + "/" + port_name;

		if (LADSPA_IS_PORT_CONTROL(m_descriptor->PortDescriptors[j]))
			port_buffer_size = 1;
		else if (LADSPA_IS_PORT_AUDIO(m_descriptor->PortDescriptors[j]))
			port_buffer_size = m_buffer_size;
		
		assert (LADSPA_IS_PORT_INPUT(m_descriptor->PortDescriptors[j])
			|| LADSPA_IS_PORT_OUTPUT(m_descriptor->PortDescriptors[j]));

		if (LADSPA_IS_PORT_INPUT(m_descriptor->PortDescriptors[j])) {
			port = new InputPort<sample>(this, port_name, j, m_poly,
				new PortInfo(port_path, m_descriptor->PortDescriptors[j],
					m_descriptor->PortRangeHints[j].HintDescriptor), port_buffer_size);
			m_ports.at(j) = port;
		} else if (LADSPA_IS_PORT_OUTPUT(m_descriptor->PortDescriptors[j])) {
			port = new OutputPort<sample>(this, port_name, j, m_poly,
				new PortInfo(port_path, m_descriptor->PortDescriptors[j],
					m_descriptor->PortRangeHints[j].HintDescriptor), port_buffer_size);
			m_ports.at(j) = port;
		}

		assert(m_ports.at(j) != NULL);

		PortInfo* pi = port->port_info();
		get_port_vals(j, pi);

		// Set default control val
		if (pi->is_control())
			((PortBase<sample>*)port)->set_value(pi->default_val(), 0);
		else
			((PortBase<sample>*)port)->set_value(0.0f, 0);
	}

	return true;
}


LADSPAPlugin::~LADSPAPlugin()
{
	for (size_t i=0; i < m_poly; ++i)
		m_descriptor->cleanup(m_instances[i]);

	delete[] m_instances;
}


void
LADSPAPlugin::activate()
{
	NodeBase::activate();

	PortBase<sample>* port = NULL;
	
	for (size_t i=0; i < m_poly; ++i) {
		for (unsigned long j=0; j < m_descriptor->PortCount; ++j) {
			port = static_cast<PortBase<sample>*>(m_ports.at(j));
			set_port_buffer(i, j, ((PortBase<sample>*)m_ports.at(j))->buffer(i)->data());
				if (port->port_info()->is_control())
					port->set_value(port->port_info()->default_val(), 0);
				else if (port->port_info()->is_audio())
					port->set_value(0.0f, 0);
		}
		if (m_descriptor->activate != NULL)
			m_descriptor->activate(m_instances[i]);
	}
}


void
LADSPAPlugin::deactivate()
{
	NodeBase::deactivate();
	
	for (size_t i=0; i < m_poly; ++i)
		if (m_descriptor->deactivate != NULL)
			m_descriptor->deactivate(m_instances[i]);
}


void
LADSPAPlugin::run(size_t nframes)
{
	NodeBase::run(nframes); // mixes down input ports
	for (size_t i=0; i < m_poly; ++i) 
		m_descriptor->run(m_instances[i], nframes);
}


void
LADSPAPlugin::set_port_buffer(size_t voice, size_t port_num, void* buf)
{
	assert(voice < m_poly);
	
	// Could be a MIDI port after this
	if (port_num < m_descriptor->PortCount) {
		m_descriptor->connect_port(m_instances[voice], port_num, (sample*)buf);
	}
}


// Based on code stolen from jack-rack
void
LADSPAPlugin::get_port_vals(ulong port_index, PortInfo* info)
{
	LADSPA_Data upper = 0.0f;
	LADSPA_Data lower = 0.0f;
	LADSPA_Data normal = 0.0f;
	LADSPA_PortRangeHintDescriptor hint_descriptor = m_descriptor->PortRangeHints[port_index].HintDescriptor;

	/* set upper and lower, possibly adjusted to the sample rate */
	if (LADSPA_IS_HINT_SAMPLE_RATE(hint_descriptor)) {
		upper = m_descriptor->PortRangeHints[port_index].UpperBound * m_srate;
		lower = m_descriptor->PortRangeHints[port_index].LowerBound * m_srate;
	} else {
		upper = m_descriptor->PortRangeHints[port_index].UpperBound;
		lower = m_descriptor->PortRangeHints[port_index].LowerBound;
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


} // namespace Om

