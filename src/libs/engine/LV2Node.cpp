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

#include <raul/Maid.hpp>
#include <iostream>
#include <cassert>
#include <float.h>
#include <stdint.h>
#include <cmath>
#include "LV2Node.hpp"
#include "InputPort.hpp"
#include "OutputPort.hpp"
#include "Plugin.hpp"
#include "AudioBuffer.hpp"
#include "MidiBuffer.hpp"
#include "OSCBuffer.hpp"

namespace Ingen {


/** Partially construct a LV2Node.
 *
 * Object is not usable until instantiate() is called with success.
 * (It _will_ crash!)
 */
LV2Node::LV2Node(const Plugin*      plugin,
                 const string&      name,
                 bool               polyphonic,
                 Patch*             parent,
                 SampleRate         srate,
                 size_t             buffer_size)
	: NodeBase(plugin, name, polyphonic, parent, srate, buffer_size)
	, _lv2_plugin(plugin->slv2_plugin())
	, _instances(NULL)
	, _prepared_instances(NULL)
{
	assert(_lv2_plugin);
}


bool
LV2Node::prepare_poly(uint32_t poly)
{
	if (!_polyphonic)
		return true;

	NodeBase::prepare_poly(poly);

	if (_prepared_instances && poly <= _prepared_instances->size())
		return true;
	
	_prepared_instances = new Raul::Array<SLV2Instance>(poly, *_instances);
	for (uint32_t i = _polyphony; i < _prepared_instances->size(); ++i) {
		_prepared_instances->at(i) = slv2_plugin_instantiate(_lv2_plugin, _srate, NULL);
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

	assert(poly <= _prepared_instances->size());
	
	if (_prepared_instances) {
		maid.push(_instances);
		_instances = _prepared_instances;

#if 0
		for (uint32_t port=0; port < num_ports(); ++port)
			for (uint32_t voice = /*_polyphony*/0; voice < _prepared_instances->size(); ++voice)
				slv2_instance_connect_port((*_instances)[voice], port,
						_ports->at(port)->buffer(voice)->raw_data());
#endif
	
		_prepared_instances = NULL;
	}

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
	uint32_t num_ports = slv2_plugin_get_num_ports(_lv2_plugin);
	assert(num_ports > 0);

	_ports = new Raul::Array<Port*>(num_ports);
	
	_instances = new Raul::Array<SLV2Instance>(_polyphony);
	
	uint32_t port_buffer_size = 0;
	
	for (uint32_t i=0; i < _polyphony; ++i) {
		(*_instances)[i] = slv2_plugin_instantiate(_lv2_plugin, _srate, NULL);
		if ((*_instances)[i] == NULL) {
			cerr << "Failed to instantiate plugin!" << endl;
			return false;
		}
	}
	
	string port_name;
	string port_path;
	
	Port* port = NULL;
	
	for (uint32_t j=0; j < num_ports; ++j) {
		SLV2Port id = slv2_plugin_get_port_by_index(_lv2_plugin, j);

		// LV2 shortnames are guaranteed to be unique, valid C identifiers
		port_name = (char*)slv2_port_get_symbol(_lv2_plugin, id);
	
		assert(port_name.find("/") == string::npos);

		port_path = path() + "/" + port_name;
		
		SLV2PortDirection port_direction = slv2_port_get_direction(_lv2_plugin, id);
		SLV2PortDataType  port_type      = slv2_port_get_data_type(_lv2_plugin, id);

		// FIXME: MIDI/OSC buffer size?
		if (port_type != SLV2_PORT_DATA_TYPE_CONTROL)
			port_buffer_size = _buffer_size;
		else
			port_buffer_size = 1;
		
		DataType data_type = DataType::UNKNOWN;
		switch (port_type) {
		case SLV2_PORT_DATA_TYPE_CONTROL:
		case SLV2_PORT_DATA_TYPE_AUDIO:
			data_type = DataType::FLOAT; break;
		case SLV2_PORT_DATA_TYPE_MIDI:
			data_type = DataType::MIDI; break;
		case SLV2_PORT_DATA_TYPE_OSC:
			data_type = DataType::OSC;
		default:
			break;
		}

		if (data_type == DataType::UNKNOWN || port_direction == SLV2_PORT_DIRECTION_UNKNOWN) {
			delete _ports;
			_ports = NULL;
			delete _instances;
			_instances = NULL;
			return false;
		}

		bool is_input = (port_direction == SLV2_PORT_DIRECTION_INPUT);

		if (is_input)
			port = new InputPort(this, port_name, j, _polyphony, data_type, port_buffer_size);
		else
			port = new OutputPort(this, port_name, j, _polyphony, data_type, port_buffer_size);

		if (is_input && port_type == SLV2_PORT_DATA_TYPE_CONTROL)
			((AudioBuffer*)port->buffer(0))->set(slv2_port_get_default_value(_lv2_plugin, id), 0);

		_ports->at(j) = port;
	}
	return true;
}


LV2Node::~LV2Node()
{
	for (uint32_t i=0; i < _polyphony; ++i)
		slv2_instance_free((*_instances)[i]);

	delete _instances;
}


void
LV2Node::activate()
{
	NodeBase::activate();

	for (uint32_t i=0; i < _polyphony; ++i) {
		for (unsigned long j=0; j < num_ports(); ++j) {
			Port* const port = _ports->at(j);
			set_port_buffer(i, j, port->buffer(i));
			if (port->type() == DataType::FLOAT && port->buffer_size() == 1) {
				((AudioBuffer*)port->buffer(i))->set(
					slv2_port_get_default_value(_lv2_plugin,
							slv2_plugin_get_port_by_index(_lv2_plugin, j)),
					0);
			} else if (port->type() == DataType::FLOAT) {
				((AudioBuffer*)port->buffer(i))->set(0.0f, 0);
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
LV2Node::process(SampleCount nframes, FrameTime start, FrameTime end)
{
	NodeBase::pre_process(nframes, start, end);

	for (uint32_t i=0; i < _polyphony; ++i) 
		slv2_instance_run((*_instances)[i], nframes);
	
	NodeBase::post_process(nframes, start, end);
}


void
LV2Node::set_port_buffer(uint32_t voice, uint32_t port_num, Buffer* buf)
{
	assert(voice < _polyphony);
	
	slv2_instance_connect_port((*_instances)[voice], port_num, buf->raw_data());
}


} // namespace Ingen

