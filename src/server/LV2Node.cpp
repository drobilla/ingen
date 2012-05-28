/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdint.h>

#include <cassert>
#include <cmath>
#include <string>

#include "lv2/lv2plug.in/ns/ext/resize-port/resize-port.h"

#include "raul/log.hpp"
#include "raul/Maid.hpp"
#include "raul/Array.hpp"

#include "ingen/shared/URIMap.hpp"
#include "ingen/shared/URIs.hpp"

#include "AudioBuffer.hpp"
#include "Engine.hpp"
#include "InputPort.hpp"
#include "LV2Node.hpp"
#include "LV2Plugin.hpp"
#include "OutputPort.hpp"
#include "ProcessContext.hpp"

using namespace std;

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
	, _worker_iface(NULL)
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

	_prepared_instances = new Instances(poly, *_instances, SharedPtr<void>());
	for (uint32_t i = _polyphony; i < _prepared_instances->size(); ++i) {
		_prepared_instances->at(i) = SharedPtr<void>(
			lilv_plugin_instantiate(
				_lv2_plugin->lilv_plugin(), _srate, _features->array()),
			lilv_instance_free);

		if (!_prepared_instances->at(i)) {
			Raul::error << "Failed to instantiate plugin" << endl;
			return false;
		}

		// Initialize the values of new ports
		for (uint32_t j = 0; j < num_ports(); ++j) {
			PortImpl* const port   = _ports->at(j);
			Buffer* const   buffer = port->prepared_buffer(i).get();
			if (buffer) {
				if (port->is_a(PortType::CV) || port->is_a(PortType::CONTROL)) {
					((AudioBuffer*)buffer)->set_value(port->value().get_float(), 0, 0);
				} else {
					buffer->clear();
				}
			}
		}

		if (_activated)
			lilv_instance_activate((LilvInstance*)(*_prepared_instances)[i].get());
	}

	return true;
}

bool
LV2Node::apply_poly(ProcessContext& context, Raul::Maid& maid, uint32_t poly)
{
	if (!_polyphonic)
		poly = 1;

	if (_prepared_instances) {
		maid.push(_instances);
		_instances = _prepared_instances;
		_prepared_instances = NULL;
	}
	assert(poly <= _instances->size());

	return NodeImpl::apply_poly(context, maid, poly);
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
	const Ingen::Shared::URIs& uris  = bufs.uris();
	SharedPtr<LV2Info>         info  = _lv2_plugin->lv2_info();
	const LilvPlugin*          plug  = _lv2_plugin->lilv_plugin();
	Ingen::Shared::Forge&      forge = bufs.forge();

	uint32_t num_ports = lilv_plugin_get_num_ports(plug);
	assert(num_ports > 0);

	_ports     = new Raul::Array<PortImpl*>(num_ports, NULL);
	_instances = new Instances(_polyphony, SharedPtr<void>());

	_features = info->world().lv2_features().lv2_features(&info->world(), this);

	uint32_t port_buffer_size = 0;
	LilvNode* work_schedule = lilv_new_uri(info->lv2_world(),
	                                       LV2_WORKER__schedule);

	for (uint32_t i = 0; i < _polyphony; ++i) {
		(*_instances)[i] = SharedPtr<void>(
			lilv_plugin_instantiate(plug, _srate, _features->array()),
			lilv_instance_free);

		if (!instance(i)) {
			Raul::error << "Failed to instantiate plugin " << _lv2_plugin->uri()
			            << " voice " << i << endl;
			return false;
		}

		if (i == 0 && lilv_plugin_has_feature(plug, work_schedule)) {
			_worker_iface = (LV2_Worker_Interface*)
				lilv_instance_get_extension_data(instance(i),
				                                 LV2_WORKER__interface);
		}
	}

	lilv_node_free(work_schedule);

	string     port_name;
	Raul::Path port_path;

	PortImpl* port = NULL;
	bool      ret  = true;

	float* min_values = new float[num_ports];
	float* max_values = new float[num_ports];
	float* def_values = new float[num_ports];
	lilv_plugin_get_port_ranges_float(plug, min_values, max_values, def_values);

	for (uint32_t j = 0; j < num_ports; ++j) {
		const LilvPort* id = lilv_plugin_get_port_by_index(plug, j);

		// LV2 port symbols are guaranteed to be unique, valid C identifiers
		port_name = lilv_node_as_string(lilv_port_get_symbol(plug, id));

		if (!Raul::Symbol::is_valid(port_name)) {
			Raul::error << "Plugin " << _lv2_plugin->uri() << " port " << j
			            << " has illegal symbol `" << port_name << "'" << endl;
			ret = false;
			break;
		}

		assert(port_name.find('/') == string::npos);

		port_path = path().child(port_name);

		Raul::Atom val;
		PortType port_type   = PortType::UNKNOWN;
		LV2_URID buffer_type = 0;
		if (lilv_port_is_a(plug, id, info->lv2_ControlPort)) {
			port_type   = PortType::CONTROL;
			buffer_type = uris.atom_Float;
		} else if (lilv_port_is_a(plug, id, info->lv2_CVPort)) {
			port_type = PortType::CV;
			buffer_type = uris.atom_Sound;
		} else if (lilv_port_is_a(plug, id, info->lv2_AudioPort)) {
			port_type = PortType::AUDIO;
			buffer_type = uris.atom_Sound;
		} else if (lilv_port_is_a(plug, id, info->atom_AtomPort)) {
			port_type = PortType::ATOM;
		}

		// Get buffer type if necessary (value and message ports)
		if (!buffer_type) {
			LilvNodes* types = lilv_port_get_value(plug, id, info->atom_bufferType);
			LILV_FOREACH(nodes, i, types) {
				const LilvNode* type = lilv_nodes_get(types, i);
				if (lilv_node_is_uri(type)) {
					port->add_property(uris.atom_bufferType,
					                   forge.alloc_uri(lilv_node_as_uri(type)));
					buffer_type = bufs.engine().world()->uri_map().map_uri(
						lilv_node_as_uri(type));
				}
			}
		}

		port_buffer_size = bufs.default_size(buffer_type);

		if (port_type == PortType::ATOM) {
			// Get default value, and its length
			LilvNodes* defaults = lilv_port_get_value(plug, id, info->lv2_default);
			LILV_FOREACH(nodes, i, defaults) {
				const LilvNode* d = lilv_nodes_get(defaults, i);
				if (lilv_node_is_string(d)) {
					const char*    str_val     = lilv_node_as_string(d);
					const uint32_t str_val_len = strlen(str_val);
					val = forge.alloc(str_val);
					port_buffer_size = std::max(port_buffer_size, str_val_len);
				}
			}

			// Get minimum size, if set in data
			LilvNodes* sizes = lilv_port_get_value(plug, id, info->rsz_minimumSize);
			LILV_FOREACH(nodes, i, sizes) {
				const LilvNode* d = lilv_nodes_get(sizes, i);
				if (lilv_node_is_int(d)) {
					uint32_t size_val = lilv_node_as_int(d);
					port_buffer_size = std::max(port_buffer_size, size_val);
				}
			}

			Raul::info << "Atom port " << path() << " buffer size "
			           << port_buffer_size << std::endl;
		}

		enum { UNKNOWN, INPUT, OUTPUT } direction = UNKNOWN;
		if (lilv_port_is_a(plug, id, info->lv2_InputPort)) {
			direction = INPUT;
		} else if (lilv_port_is_a(plug, id, info->lv2_OutputPort)) {
			direction = OUTPUT;
		}

		if (port_type == PortType::UNKNOWN || direction == UNKNOWN) {
			Raul::warn << "Unknown type or direction for port `" << port_name << "'" << endl;
			ret = false;
			break;
		}

		if (!val.type())
			val = forge.make(isnan(def_values[j]) ? 0.0f : def_values[j]);

		// TODO: set buffer size when necessary
		if (direction == INPUT)
			port = new InputPort(bufs, this, port_name, j, _polyphony, port_type, buffer_type, val);
		else
			port = new OutputPort(bufs, this, port_name, j, _polyphony, port_type, buffer_type, val);

		if (direction == INPUT && (port_type == PortType::CONTROL
		                           || port_type == PortType::CV)) {
			port->set_value(val);
			if (!isnan(min_values[j])) {
				port->set_property(uris.lv2_minimum, forge.make(min_values[j]));
				port->set_minimum(forge.make(min_values[j]));
			}
			if (!isnan(max_values[j])) {
				port->set_property(uris.lv2_maximum, forge.make(max_values[j]));
				port->set_maximum(forge.make(max_values[j]));
			}
		}

		// Set lv2:portProperty properties
		LilvNodes* properties = lilv_port_get_value(plug, id, info->lv2_portProperty);
		LILV_FOREACH(nodes, i, properties) {
			const LilvNode* p = lilv_nodes_get(properties, i);
			if (lilv_node_is_uri(p)) {
				port->add_property(uris.lv2_portProperty,
				                   forge.alloc_uri(lilv_node_as_uri(p)));
			}
		}

		// Set atom:supports properties
		LilvNodes* types = lilv_port_get_value(plug, id, info->atom_supports);
		LILV_FOREACH(nodes, i, types) {
			const LilvNode* type = lilv_nodes_get(types, i);
			if (lilv_node_is_uri(type)) {
				port->add_property(uris.atom_supports,
				                   forge.alloc_uri(lilv_node_as_uri(type)));
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

	return ret;
}

void
LV2Node::activate(BufferFactory& bufs)
{
	NodeImpl::activate(bufs);

	for (uint32_t i = 0; i < _polyphony; ++i)
		lilv_instance_activate(instance(i));
}

void
LV2Node::deactivate()
{
	NodeImpl::deactivate();

	for (uint32_t i = 0; i < _polyphony; ++i)
		lilv_instance_deactivate(instance(i));
}

void
LV2Node::work(MessageContext& context, uint32_t size, const void* data)
{
	if (_worker_iface) {
		_worker_iface->work(instance(0), NULL, NULL, size, data);
	}
}

void
LV2Node::process(ProcessContext& context)
{
	NodeImpl::pre_process(context);

	for (uint32_t i = 0; i < _polyphony; ++i)
		lilv_instance_run(instance(i), context.nframes());

	NodeImpl::post_process(context);
}

void
LV2Node::set_port_buffer(uint32_t    voice,
                         uint32_t    port_num,
                         BufferRef   buf,
                         SampleCount offset)
{
	NodeImpl::set_port_buffer(voice, port_num, buf, offset);
	lilv_instance_connect_port(
		instance(voice), port_num,
	    buf ? buf->port_data(_ports->at(port_num)->type(), offset) : NULL);
}

} // namespace Server
} // namespace Ingen

