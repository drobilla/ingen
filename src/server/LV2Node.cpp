/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
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

#include <float.h>
#include <stdint.h>

#include <cassert>
#include <cmath>
#include <string>

#include "raul/log.hpp"
#include "raul/Maid.hpp"
#include "raul/Array.hpp"

#include "AudioBuffer.hpp"
#include "EventBuffer.hpp"
#include "InputPort.hpp"
#include "LV2Node.hpp"
#include "LV2Plugin.hpp"
#include "LV2URIMap.hpp"
#include "MessageContext.hpp"
#include "OutputPort.hpp"
#include "ProcessContext.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Server {

/** Partially construct a LV2Node.
 *
 * Object is not usable until instantiate() is called with success.
 * (It _will_ crash!)
 */
LV2Node::LV2Node(LV2Plugin*    plugin,
				 const string& name,
				 bool          polyphonic,
				 PatchImpl*    parent,
				 SampleRate    srate)
	: NodeImpl(plugin, name, polyphonic, parent, srate)
	, _lv2_plugin(plugin)
	, _instances(NULL)
	, _prepared_instances(NULL)
	, _message_funcs(NULL)
{
	assert(_lv2_plugin);
}

LV2Node::~LV2Node()
{
	delete _instances;
}

bool
LV2Node::prepare_poly(BufferFactory& bufs, uint32_t poly)
{
	if (!_polyphonic)
		poly = 1;

	NodeImpl::prepare_poly(bufs, poly);

	if (_polyphony == poly)
		return true;

	SharedPtr<LV2Info> info = _lv2_plugin->lv2_info();
	_prepared_instances = new Instances(poly, *_instances, SharedPtr<void>());
	for (uint32_t i = _polyphony; i < _prepared_instances->size(); ++i) {
		_prepared_instances->at(i) = SharedPtr<void>(
				slv2_plugin_instantiate(
					_lv2_plugin->slv2_plugin(), _srate, _features->array()),
				slv2_instance_free);

		if (!_prepared_instances->at(i)) {
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

		if (_activated)
			slv2_instance_activate((SLV2Instance)(*_prepared_instances)[i].get());
	}

	return true;
}

bool
LV2Node::apply_poly(Raul::Maid& maid, uint32_t poly)
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

/** Instantiate self from LV2 plugin descriptor.
 *
 * Implemented as a seperate function (rather than in the constructor) to
 * allow graceful error-catching of broken plugins.
 *
 * Returns whether or not plugin was successfully instantiated.  If return
 * value is false, this object may not be used.
 */
bool
LV2Node::instantiate(BufferFactory& bufs)
{
	const Ingen::Shared::LV2URIMap& uris = bufs.uris();
	SharedPtr<LV2Info>              info = _lv2_plugin->lv2_info();
	SLV2Plugin                      plug = _lv2_plugin->slv2_plugin();

	uint32_t num_ports = slv2_plugin_get_num_ports(plug);
	assert(num_ports > 0);

	_ports     = new Raul::Array<PortImpl*>(num_ports, NULL);
	_instances = new Instances(_polyphony, SharedPtr<void>());

	_features = info->world().lv2_features()->lv2_features(&info->world(), this);

	uint32_t port_buffer_size = 0;
	SLV2Value ctx_ext_uri = slv2_value_new_uri(info->lv2_world(),
	                                           LV2_CONTEXTS_URI "#MessageContext");

	for (uint32_t i = 0; i < _polyphony; ++i) {
		(*_instances)[i] = SharedPtr<void>(
				slv2_plugin_instantiate(plug, _srate, _features->array()),
				slv2_instance_free);

		if (!instance(i)) {
			error << "Failed to instantiate plugin " << _lv2_plugin->uri()
			      << " voice " << i << endl;
			return false;
		}

		if (!slv2_plugin_has_feature(plug, ctx_ext_uri))
			continue;

		const void* ctx_ext = slv2_instance_get_extension_data(
			instance(i), LV2_CONTEXTS_URI "#MessageContext");

		if (i == 0 && ctx_ext) {
			assert(!_message_funcs);
			_message_funcs = (LV2_Contexts_MessageContext*)ctx_ext;
		}
	}

	slv2_value_free(ctx_ext_uri);

	string port_name;
	Path   port_path;

	PortImpl* port = NULL;
	bool      ret  = true;

	float* min_values = new float[num_ports];
	float* max_values = new float[num_ports];
	float* def_values = new float[num_ports];
	slv2_plugin_get_port_ranges_float(plug, min_values, max_values, def_values);

	SLV2Value context_pred = slv2_value_new_uri(info->lv2_world(),
			"http://lv2plug.in/ns/ext/contexts#context");

	SLV2Value default_pred = slv2_value_new_uri(info->lv2_world(),
			"http://lv2plug.in/ns/lv2core#default");

	SLV2Value min_size_pred = slv2_value_new_uri(info->lv2_world(),
			"http://lv2plug.in/ns/ext/resize-port#minimumSize");

	SLV2Value port_property_pred = slv2_value_new_uri(info->lv2_world(),
			"http://lv2plug.in/ns/lv2core#portProperty");

	SLV2Value supports_pred = slv2_value_new_uri(info->lv2_world(),
			"http://lv2plug.in/ns/ext/atom#supports");

	//SLV2Value as_large_as_pred = slv2_value_new_uri(info->lv2_world(),
	//		"http://lv2plug.in/ns/ext/resize-port#asLargeAs");

	for (uint32_t j = 0; j < num_ports; ++j) {
		SLV2Port id = slv2_plugin_get_port_by_index(plug, j);

		// LV2 port symbols are guaranteed to be unique, valid C identifiers
		port_name = slv2_value_as_string(slv2_port_get_symbol(plug, id));

		if (!Symbol::is_valid(port_name)) {
			error << "Plugin " << _lv2_plugin->uri() << " port " << j
				<< " has illegal symbol `" << port_name << "'" << endl;
			ret = false;
			break;
		}

		assert(port_name.find('/') == string::npos);

		port_path = path().child(port_name);

		Raul::Atom val;
		PortType data_type = PortType::UNKNOWN;
		if (slv2_port_is_a(plug, id, info->control_class)) {
			data_type = PortType::CONTROL;
		} else if (slv2_port_is_a(plug, id, info->audio_class)) {
			data_type = PortType::AUDIO;
		} else if (slv2_port_is_a(plug, id, info->event_class)) {
			data_type = PortType::EVENTS;
		} else if (slv2_port_is_a(plug, id, info->value_port_class)) {
			data_type = PortType::VALUE;
		} else if (slv2_port_is_a(plug, id, info->message_port_class)) {
			data_type = PortType::MESSAGE;
		}

		port_buffer_size = bufs.default_buffer_size(data_type);

		if (data_type == PortType::VALUE || data_type == PortType::MESSAGE) {
			// Get default value, and its length
			SLV2Values defaults = slv2_port_get_value(plug, id, default_pred);
			SLV2_FOREACH(i, defaults) {
				SLV2Value d = slv2_values_get(defaults, i);
				if (slv2_value_is_string(d)) {
					const char*  str_val     = slv2_value_as_string(d);
					const size_t str_val_len = strlen(str_val);
					val = str_val;
					port_buffer_size = str_val_len;
				}
			}

			// Get minimum size, if set in data
			SLV2Values sizes = slv2_port_get_value(plug, id, min_size_pred);
			SLV2_FOREACH(i, sizes) {
				SLV2Value d = slv2_values_get(sizes, i);
				if (slv2_value_is_int(d)) {
					size_t size_val = slv2_value_as_int(d);
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

		if (data_type == PortType::UNKNOWN || direction == UNKNOWN) {
			warn << "Unknown type or direction for port `" << port_name << "'" << endl;
			ret = false;
			break;
		}

		if (val.type() == Atom::NIL)
			val = isnan(def_values[j]) ? 0.0f : def_values[j];

		if (direction == INPUT)
			port = new InputPort(bufs, this, port_name, j, _polyphony, data_type, val);
		else
			port = new OutputPort(bufs, this, port_name, j, _polyphony, data_type, val);

		if (direction == INPUT && data_type == PortType::CONTROL) {
			port->set_value(val);
			if (!isnan(min_values[j])) {
				port->set_property(uris.lv2_minimum, min_values[j]);
			}
			if (!isnan(max_values[j])) {
				port->set_property(uris.lv2_maximum, max_values[j]);
			}
		}

		// Set lv2:portProperty properties
		SLV2Values properties = slv2_port_get_value(plug, id, port_property_pred);
		SLV2_FOREACH(i, properties) {
			SLV2Value p = slv2_values_get(properties, i);
			if (slv2_value_is_uri(p)) {
				port->set_property(uris.lv2_portProperty, Raul::URI(slv2_value_as_uri(p)));
			}
		}

		// Set atom:supports properties
		SLV2Values types = slv2_port_get_value(plug, id, supports_pred);
		SLV2_FOREACH(i, types) {
			SLV2Value type = slv2_values_get(types, i);
			if (slv2_value_is_uri(type)) {
				port->add_property(uris.atom_supports, Raul::URI(slv2_value_as_uri(type)));
			}
		}

		SLV2Values contexts = slv2_port_get_value(plug, id, context_pred);
		SLV2_FOREACH(i, contexts) {
			SLV2Value c = slv2_values_get(contexts, i);
			const char* context = slv2_value_as_string(c);
			if (!strcmp(LV2_CONTEXTS_URI "#MessageContext", context)) {
				if (!_message_funcs) {
					warn << _lv2_plugin->uri()
							<< " has a message port, but no context extension data." << endl;
				}
				port->set_context(Context::MESSAGE);
			} else {
				warn << _lv2_plugin->uri() << " port " << i << " has unknown context "
				     << slv2_value_as_string(c)
				     << endl;
			}
		}

		_ports->at(j) = port;
	}

	if (!ret) {
		delete _ports;
		_ports = NULL;
		delete _instances;
		_instances = NULL;
	}

	delete[] min_values;
	delete[] max_values;
	delete[] def_values;
	slv2_value_free(context_pred);
	slv2_value_free(default_pred);
	slv2_value_free(min_size_pred);
	slv2_value_free(port_property_pred);

	return ret;
}

void
LV2Node::activate(BufferFactory& bufs)
{
	NodeImpl::activate(bufs);

	for (uint32_t i = 0; i < _polyphony; ++i)
		slv2_instance_activate(instance(i));
}

void
LV2Node::deactivate()
{
	NodeImpl::deactivate();

	for (uint32_t i = 0; i < _polyphony; ++i)
		slv2_instance_deactivate(instance(i));
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
		(*_message_funcs->run)(instance(0)->lv2_handle, _valid_ports, _valid_ports);
}

void
LV2Node::process(ProcessContext& context)
{
	NodeImpl::pre_process(context);

	for (uint32_t i = 0; i < _polyphony; ++i)
		slv2_instance_run(instance(i), context.nframes());

	NodeImpl::post_process(context);
}

void
LV2Node::set_port_buffer(uint32_t voice, uint32_t port_num,
		IntrusivePtr<Buffer> buf, SampleCount offset)
{
	NodeImpl::set_port_buffer(voice, port_num, buf, offset);
	slv2_instance_connect_port(instance(voice), port_num,
			buf ? buf->port_data(_ports->at(port_num)->buffer_type(), offset) : NULL);
}

} // namespace Server
} // namespace Ingen
