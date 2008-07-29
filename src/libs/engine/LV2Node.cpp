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
#include <float.h>
#include <stdint.h>
#include <cmath>
#include <raul/Maid.hpp>
#include "AudioBuffer.hpp"
#include "InputPort.hpp"
#include "LV2Node.hpp"
#include "LV2Plugin.hpp"
#include "EventBuffer.hpp"
#include "OutputPort.hpp"
#include "ProcessContext.hpp"

using namespace std;

namespace Ingen {


/** Partially construct a LV2Node.
 *
 * Object is not usable until instantiate() is called with success.
 * (It _will_ crash!)
 */
LV2Node::LV2Node(LV2Plugin*    plugin,
                 const string& name,
                 bool          polyphonic,
                 PatchImpl*    parent,
                 SampleRate    srate,
                 size_t        buffer_size)
	: NodeBase(plugin, name, polyphonic, parent, srate, buffer_size)
	, _lv2_plugin(plugin)
	, _instances(NULL)
	, _prepared_instances(NULL)
{
	assert(_lv2_plugin);
}


LV2Node::~LV2Node()
{
	for (uint32_t i=0; i < _polyphony; ++i)
		slv2_instance_free((*_instances)[i]);

	delete _instances;
}


bool
LV2Node::prepare_poly(uint32_t poly)
{
	NodeBase::prepare_poly(poly);
	
	if ( (!_polyphonic)
			|| (_prepared_instances && poly <= _prepared_instances->size()) ) {
		return true;
	}

	_prepared_instances = new Raul::Array<SLV2Instance>(poly, *_instances);
	for (uint32_t i = _polyphony; i < _prepared_instances->size(); ++i) {
		// FIXME: features array (in NodeFactory) must be passed!
		_prepared_instances->at(i) = slv2_plugin_instantiate(
				_lv2_plugin->slv2_plugin(), _srate, NULL);

		if ((*_prepared_instances)[i] == NULL) {
			cerr << "Failed to instantiate plugin!" << endl;
			return false;
		}

		if (_activated)
			slv2_instance_activate((*_prepared_instances)[i]);
	}
	
	return true;
}


bool
LV2Node::apply_poly(Raul::Maid& maid, uint32_t poly)
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
	SharedPtr<LV2Info> info = _lv2_plugin->lv2_info();
	SLV2Plugin         plug = _lv2_plugin->slv2_plugin();

	uint32_t num_ports = slv2_plugin_get_num_ports(plug);
	assert(num_ports > 0);

	_ports = new Raul::Array<PortImpl*>(num_ports, NULL);
	
	_instances = new Raul::Array<SLV2Instance>(_polyphony, NULL);
	
	uint32_t port_buffer_size = 0;
	
	for (uint32_t i=0; i < _polyphony; ++i) {
		(*_instances)[i] = slv2_plugin_instantiate(plug, _srate, info->lv2_features());
		if ((*_instances)[i] == NULL) {
			cerr << "Failed to instantiate plugin!" << endl;
			return false;
		}
	}
	
	string port_name;
	string port_path;
	
	PortImpl* port = NULL;
	
	float* def_values = new float[num_ports];
	slv2_plugin_get_port_ranges_float(plug, 0, 0, def_values);
	
	for (uint32_t j=0; j < num_ports; ++j) {
		SLV2Port id = slv2_plugin_get_port_by_index(plug, j);

		// LV2 shortnames are guaranteed to be unique, valid C identifiers
		port_name = slv2_value_as_string(slv2_port_get_symbol(plug, id));
	
		assert(port_name.find("/") == string::npos);

		port_path = path() + "/" + port_name;
		
		DataType data_type = DataType::UNKNOWN;
		if (slv2_port_is_a(plug, id, info->control_class)) {
			data_type = DataType::CONTROL;
			port_buffer_size = 1;
		} else if (slv2_port_is_a(plug, id, info->audio_class)) {
			data_type = DataType::AUDIO;
			port_buffer_size = _buffer_size;
		} else if (slv2_port_is_a(plug, id, info->event_class)) {
			data_type = DataType::EVENT;
			port_buffer_size = _buffer_size;
		}

		enum { UNKNOWN, INPUT, OUTPUT } direction = UNKNOWN;
		if (slv2_port_is_a(plug, id, info->input_class)) {
			direction = INPUT;
		} else if (slv2_port_is_a(plug, id, info->output_class)) {
			direction = OUTPUT;
		}

		if (data_type == DataType::UNKNOWN || direction == UNKNOWN) {
			delete _ports;
			_ports = NULL;
			delete _instances;
			_instances = NULL;
			return false;
		}
			
		// FIXME: need nice type preserving SLV2Value -> Raul::Atom conversion
		float def = isnan(def_values[j]) ? 0.0f : def_values[j];
		Atom defatm = def;

		if (direction == INPUT)
			port = new InputPort(this, port_name, j, _polyphony, data_type, defatm, port_buffer_size);
		else
			port = new OutputPort(this, port_name, j, _polyphony, data_type, defatm, port_buffer_size);

		if (direction == INPUT && data_type == DataType::CONTROL)
		  ((AudioBuffer*)port->buffer(0))->set_value(def, 0, 0);

		_ports->at(j) = port;
	}
	
	delete [] def_values;
	
	return true;
}


void
LV2Node::activate()
{
	NodeBase::activate();

	for (uint32_t i=0; i < _polyphony; ++i) {
		for (unsigned long j=0; j < num_ports(); ++j) {
			PortImpl* const port = _ports->at(j);

			set_port_buffer(i, j, port->buffer(i));

			if (port->type() == DataType::CONTROL) {
				((AudioBuffer*)port->buffer(i))->set_value(port->value().get_float(), 0, 0);
			} else if (port->type() == DataType::AUDIO) {
				((AudioBuffer*)port->buffer(i))->set_value(0.0f, 0, 0);
			}
		}
		slv2_instance_activate((*_instances)[i]);
	}
}


void
LV2Node::deactivate()
{
	NodeBase::deactivate();
	
	for (uint32_t i=0; i < _polyphony; ++i)
		slv2_instance_deactivate((*_instances)[i]);
}


void
LV2Node::process(ProcessContext& context)
{
	NodeBase::pre_process(context);

	for (uint32_t i=0; i < _polyphony; ++i) 
		slv2_instance_run((*_instances)[i], context.nframes());
	
	NodeBase::post_process(context);
}


void
LV2Node::set_port_buffer(uint32_t voice, uint32_t port_num, Buffer* buf)
{
	assert(voice < _polyphony);
	
	slv2_instance_connect_port((*_instances)[voice], port_num, buf->raw_data());
}


} // namespace Ingen

