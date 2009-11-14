/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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
#include "raul/Maid.hpp"
#include "AudioBuffer.hpp"
#include "InputPort.hpp"
#include "LV2Node.hpp"
#include "LV2Plugin.hpp"
#include "EventBuffer.hpp"
#include "OutputPort.hpp"
#include "ProcessContext.hpp"
#include "MessageContext.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {

using namespace Shared;


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
	, _message_funcs(NULL)
{
	assert(_lv2_plugin);
}


LV2Node::~LV2Node()
{
	if (_instances)
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

	SharedPtr<LV2Info> info = _lv2_plugin->lv2_info();
	_prepared_instances = new Raul::Array<SLV2Instance>(poly, *_instances);
	for (uint32_t i = _polyphony; i < _prepared_instances->size(); ++i) {
		_prepared_instances->at(i) = slv2_plugin_instantiate(
				_lv2_plugin->slv2_plugin(), _srate, _features->array());

		if (_prepared_instances->at(i) == NULL) {
			cerr << "Failed to instantiate plugin!" << endl;
			return false;
		}

		// Initialize the values of new ports
		for (unsigned long j=0; j < num_ports(); ++j) {
			PortImpl* const port = _ports->at(j);
			Buffer *buffer = port->prepared_buffer(i);

			// FIXME: Preserve individual voice values
			if (port->type() == DataType::CONTROL) {
				((AudioBuffer*)buffer)->set_value(port->value().get_float(), 0, 0);
			} else if (port->type() == DataType::AUDIO) {
				((AudioBuffer*)buffer)->set_value(0.0f, 0, 0);
			}
		}

		if (_activated)
			slv2_instance_activate(_prepared_instances->at(i));
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

	return NodeBase::apply_poly(maid, poly);
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

	_ports     = new Raul::Array<PortImpl*>(num_ports, NULL);
	_instances = new Raul::Array<SLV2Instance>(_polyphony, NULL);

	_features = info->world().lv2_features->lv2_features(this);

	uint32_t port_buffer_size = 0;
	SLV2Value ctx_ext_uri = slv2_value_new_uri(info->lv2_world(), LV2_CONTEXT_MESSAGE);

	for (uint32_t i=0; i < _polyphony; ++i) {
		(*_instances)[i] = slv2_plugin_instantiate(plug, _srate, _features->array());
		if ((*_instances)[i] == NULL) {
			cerr << "Failed to instantiate plugin!" << endl;
			return false;
		}

		if (!slv2_plugin_has_feature(plug, ctx_ext_uri))
			continue;

		const void* ctx_ext = slv2_instance_get_extension_data(
				(*_instances)[i], LV2_CONTEXT_MESSAGE);

		if (i == 0 && ctx_ext) {
			cerr << _lv2_plugin->uri() << " has message context" << endl;
			assert(!_message_funcs);
			_message_funcs = (LV2MessageContext*)ctx_ext;
		}
	}

	slv2_value_free(ctx_ext_uri);

	string port_name;
	Path   port_path;

	PortImpl* port = NULL;

	float* def_values = new float[num_ports];
	slv2_plugin_get_port_ranges_float(plug, 0, 0, def_values);

	SLV2Value context_pred = slv2_value_new_uri(info->lv2_world(),
			"http://lv2plug.in/ns/dev/contexts#context");

	// FIXME: Why doesn't this just use lv2:default?
	SLV2Value default_pred = slv2_value_new_uri(info->lv2_world(),
			"http://lv2plug.in/ns/dev/string-port#default");

	// FIXME: Make this a separate extension
	SLV2Value size_pred = slv2_value_new_uri(info->lv2_world(),
			"http://lv2plug.in/ns/dev/string-port#requiredSpace");

	for (uint32_t j=0; j < num_ports; ++j) {
		SLV2Port id = slv2_plugin_get_port_by_index(plug, j);

		// LV2 port symbols are guaranteed to be unique, valid C identifiers
		port_name = slv2_value_as_string(slv2_port_get_symbol(plug, id));
		assert(port_name.find("/") == string::npos);

		port_path = path().child(port_name);

		Raul::Atom val;
		DataType data_type = DataType::UNKNOWN;
		if (slv2_port_is_a(plug, id, info->control_class)) {
			data_type = DataType::CONTROL;
			port_buffer_size = 1;
		} else if (slv2_port_is_a(plug, id, info->audio_class)) {
			data_type = DataType::AUDIO;
			port_buffer_size = _buffer_size;
		} else if (slv2_port_is_a(plug, id, info->event_class)) {
			data_type = DataType::EVENTS;
			port_buffer_size = _buffer_size;
		} else if (slv2_port_is_a(plug, id, info->object_port_class)) {
			data_type = DataType::OBJECT;
			port_buffer_size = 0;

			// Get default value, and its length
			SLV2Values defaults = slv2_port_get_value(plug, id, default_pred);
			for (uint32_t i = 0; i < slv2_values_size(defaults); ++i) {
				SLV2Value d = slv2_values_get_at(defaults, i);
				if (slv2_value_is_string(d)) {
					const char*  str_val     = slv2_value_as_string(d);
					const size_t str_val_len = strlen(str_val);
					if (str_val_len >= port_buffer_size) {
						val = str_val;
						port_buffer_size = str_val_len;
					}
				}
			}

			// Get minimum size, if set in data
			SLV2Values sizes = slv2_port_get_value(plug, id, size_pred);
			for (uint32_t i = 0; i < slv2_values_size(sizes); ++i) {
				SLV2Value d = slv2_values_get_at(sizes, i);
				if (slv2_value_is_int(d)) {
					size_t size_val = slv2_value_as_int(d);
					if (size_val > port_buffer_size)
						port_buffer_size = size_val;
				}
			}
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

		if (val.type() == Atom::NIL)
			val = isnan(def_values[j]) ? 0.0f : def_values[j];

		if (direction == INPUT)
			port = new InputPort(this, port_name, j, _polyphony, data_type, val, port_buffer_size);
		else
			port = new OutputPort(this, port_name, j, _polyphony, data_type, val, port_buffer_size);

		if (direction == INPUT && data_type == DataType::CONTROL)
			((AudioBuffer*)port->buffer(0))->set_value(val.get_float(), 0, 0);

		SLV2Values contexts = slv2_port_get_value(plug, id, context_pred);
		for (uint32_t i = 0; i < slv2_values_size(contexts); ++i) {
			SLV2Value c = slv2_values_get_at(contexts, i);
			const char* context = slv2_value_as_string(c);
			if (!strcmp(LV2_CONTEXT_MESSAGE, context)) {
				cerr << _lv2_plugin->uri() << " port " << i << " has message context" << endl;
				if (!_message_funcs) {
					cerr << _lv2_plugin->uri()
						<< " has a message port, but no context extension data." << endl;
				}
				port->set_context(Context::MESSAGE);
			} else {
				cout << _lv2_plugin->uri() << " port " << i << " has unknown context "
					<< slv2_value_as_string(slv2_values_get_at(contexts, i))
					<< endl;
			}
		}

		_ports->at(j) = port;
	}

	delete[] def_values;

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
LV2Node::message_run(MessageContext& context)
{
	for (size_t i = 0; i < num_ports(); ++i) {
		PortImpl* const port = _ports->at(i);
		if (port->context() == Context::MESSAGE)
			port->pre_process(context);
	}

	if (!_valid_ports)
		_valid_ports = calloc(num_ports() / 8, 1);
	if (_message_funcs)
		(*_message_funcs->message_run)((*_instances)[0]->lv2_handle, _valid_ports, _valid_ports);
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

