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
#include "lv2/lv2plug.in/ns/ext/morph/morph.h"

#include "raul/log.hpp"
#include "raul/Maid.hpp"
#include "raul/Array.hpp"

#include "ingen/URIMap.hpp"
#include "ingen/URIs.hpp"

#include "Driver.hpp"
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

SharedPtr<LilvInstance>
LV2Node::make_instance(URIs&      uris,
                       SampleRate rate,
                       uint32_t   voice,
                       bool       preparing)
{
	LilvInstance* inst = lilv_plugin_instantiate(
		_lv2_plugin->lilv_plugin(), rate, _features->array());

	if (!inst) {
		Raul::error(Raul::fmt("Failed to instantiate <%1%>\n")
		            % _lv2_plugin->uri());
		return SharedPtr<LilvInstance>();
	}

	const LV2_Morph_Interface* morph_iface = (const LV2_Morph_Interface*)
		lilv_instance_get_extension_data(inst, LV2_MORPH__interface);

	for (uint32_t p = 0; p < num_ports(); ++p) {
		PortImpl* const port   = _ports->at(p);
		Buffer* const   buffer = (preparing)
			? port->prepared_buffer(voice).get()
			: port->buffer(voice).get();
		if (port->is_morph() && port->is_a(PortType::CV)) {
			Raul::info(Raul::fmt("Morphing %1% to CV\n") % port->path());
			if (morph_iface) {
				morph_iface->morph_port(
					inst->lv2_handle, p, uris.lv2_CVPort, NULL);
			}
		}

		if (buffer) {
			if (port->is_a(PortType::CV) || port->is_a(PortType::CONTROL)) {
				buffer->set_block(port->value().get_float(), 0, buffer->nframes());
			} else {
				buffer->clear();
			}
		}
	}

	if (morph_iface) {
		for (uint32_t p = 0; p < num_ports(); ++p) {
			PortImpl* const port = _ports->at(p);
			if (port->is_auto_morph()) {
				LV2_URID type = morph_iface->port_type(
					inst->lv2_handle, p, NULL);
				if (type == _uris.lv2_ControlPort) {
					port->set_type(PortType::CONTROL, 0);
					Raul::info(Raul::fmt("Auto-morphed %1% to control\n")
					           % port->path());
				} else if (type == _uris.lv2_CVPort) {
					port->set_type(PortType::CV, 0);
					Raul::info(Raul::fmt("Auto-morphed %1% to CV\n")
					           % port->path());
				} else {
					Raul::error(Raul::fmt("%1% auto-morphed to unknown type %2%\n")
					            % port->path() % type);
					return SharedPtr<LilvInstance>();
				}
			}
		}
	}

	return SharedPtr<LilvInstance>(inst, lilv_instance_free);
}

bool
LV2Node::prepare_poly(BufferFactory& bufs, uint32_t poly)
{
	if (!_polyphonic)
		poly = 1;

	NodeImpl::prepare_poly(bufs, poly);

	if (_polyphony == poly)
		return true;

	const SampleRate rate = bufs.engine().driver()->sample_rate();
	assert(!_prepared_instances);
	_prepared_instances = new Instances(poly, *_instances, SharedPtr<void>());
	for (uint32_t i = _polyphony; i < _prepared_instances->size(); ++i) {
		SharedPtr<LilvInstance> inst = make_instance(bufs.uris(), rate, i, true);
		if (!inst) {
			return false;
		}

		_prepared_instances->at(i) = inst;

		if (_activated) {
			lilv_instance_activate(inst.get());
		}
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
	const Ingen::URIs& uris      = bufs.uris();
	SharedPtr<LV2Info> info      = _lv2_plugin->lv2_info();
	const LilvPlugin*  plug      = _lv2_plugin->lilv_plugin();
	Ingen::Forge&      forge     = bufs.forge();
	const uint32_t     num_ports = lilv_plugin_get_num_ports(plug);

	_ports = new Raul::Array<PortImpl*>(num_ports, NULL);

	bool ret = true;

	float* min_values = new float[num_ports];
	float* max_values = new float[num_ports];
	float* def_values = new float[num_ports];
	lilv_plugin_get_port_ranges_float(plug, min_values, max_values, def_values);

	// Get all the necessary information about ports
	for (uint32_t j = 0; j < num_ports; ++j) {
		const LilvPort* id = lilv_plugin_get_port_by_index(plug, j);

		// LV2 port symbols are guaranteed to be unique, valid C identifiers
		const std::string port_sym = lilv_node_as_string(
			lilv_port_get_symbol(plug, id));

		if (!Raul::Symbol::is_valid(port_sym)) {
			Raul::error(Raul::fmt("<%1%> port %2% has invalid symbol `%3'\n")
			            % _lv2_plugin->uri() % j % port_sym);
			ret = false;
			break;
		}

		// Get port type
		Raul::Atom val;
		PortType   port_type     = PortType::UNKNOWN;
		LV2_URID   buffer_type   = 0;
		bool       is_morph      = false;
		bool       is_auto_morph = false;
		if (lilv_port_is_a(plug, id, info->lv2_ControlPort)) {
			if (lilv_port_is_a(plug, id, info->morph_MorphPort)) {
				is_morph = true;
				LilvNodes* types = lilv_port_get_value(
					plug, id, info->morph_supportsType);
				LILV_FOREACH(nodes, i, types) {
					const LilvNode* type = lilv_nodes_get(types, i);
					if (lilv_node_equals(type, info->lv2_CVPort)) {
						port_type   = PortType::CV;
						buffer_type = uris.atom_Sound;
					}
				}
				lilv_nodes_free(types);
			}
			if (port_type == PortType::UNKNOWN) {
				port_type   = PortType::CONTROL;
				buffer_type = uris.atom_Float;
			}
		} else if (lilv_port_is_a(plug, id, info->lv2_CVPort)) {
			port_type   = PortType::CV;
			buffer_type = uris.atom_Sound;
		} else if (lilv_port_is_a(plug, id, info->lv2_AudioPort)) {
			port_type   = PortType::AUDIO;
			buffer_type = uris.atom_Sound;
		} else if (lilv_port_is_a(plug, id, info->atom_AtomPort)) {
			port_type = PortType::ATOM;
		}

		if (lilv_port_is_a(plug, id, info->morph_AutoMorphPort)) {
			is_auto_morph = true;
		}

		// Get buffer type if necessary (atom ports)
		if (!buffer_type) {
			LilvNodes* types = lilv_port_get_value(
				plug, id, info->atom_bufferType);
			LILV_FOREACH(nodes, i, types) {
				const LilvNode* type = lilv_nodes_get(types, i);
				if (lilv_node_is_uri(type)) {
					buffer_type = bufs.engine().world()->uri_map().map_uri(
						lilv_node_as_uri(type));
				}
			}
			lilv_nodes_free(types);
		}

		uint32_t port_buffer_size = bufs.default_size(buffer_type);

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
			lilv_nodes_free(defaults);

			// Get minimum size, if set in data
			LilvNodes* sizes = lilv_port_get_value(plug, id, info->rsz_minimumSize);
			LILV_FOREACH(nodes, i, sizes) {
				const LilvNode* d = lilv_nodes_get(sizes, i);
				if (lilv_node_is_int(d)) {
					uint32_t size_val = lilv_node_as_int(d);
					port_buffer_size = std::max(port_buffer_size, size_val);
				}
			}
			lilv_nodes_free(sizes);

			Raul::info(Raul::fmt("Atom port %1% buffer size %2%\n")
			           % path() % port_buffer_size);
		}

		enum { UNKNOWN, INPUT, OUTPUT } direction = UNKNOWN;
		if (lilv_port_is_a(plug, id, info->lv2_InputPort)) {
			direction = INPUT;
		} else if (lilv_port_is_a(plug, id, info->lv2_OutputPort)) {
			direction = OUTPUT;
		}

		if (port_type == PortType::UNKNOWN || direction == UNKNOWN) {
			Raul::error(Raul::fmt("<%1%> port %2% has unknown type or direction\n")
			            % _lv2_plugin->uri() % port_sym);
			ret = false;
			break;
		}

		if (!val.type())
			val = forge.make(isnan(def_values[j]) ? 0.0f : def_values[j]);

		PortImpl* port = (direction == INPUT)
			? static_cast<PortImpl*>(
				new InputPort(bufs, this, port_sym, j, _polyphony,
				              port_type, buffer_type, val))
			: static_cast<PortImpl*>(
				new OutputPort(bufs, this, port_sym, j, _polyphony,
				               port_type, buffer_type, val));

		port->set_morphable(is_morph, is_auto_morph);
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

		// Inherit certain properties from plugin port
		LilvNode* preds[] = { info->lv2_portProperty, info->atom_supports, 0 };
		for (int p = 0; preds[p]; ++p) {
			LilvNodes* values = lilv_port_get_value(plug, id, preds[p]);
			LILV_FOREACH(nodes, v, values) {
				const LilvNode* val = lilv_nodes_get(values, v);
				if (lilv_node_is_uri(val)) {
					port->add_property(lilv_node_as_uri(preds[p]),
					                   forge.alloc_uri(lilv_node_as_uri(val)));
				}
			}
			lilv_nodes_free(values);
		}

		port->cache_properties();

		_ports->at(j) = port;
	}

	delete[] min_values;
	delete[] max_values;
	delete[] def_values;

	if (!ret) {
		delete _ports;
		_ports = NULL;
		return ret;
	}

	_features = info->world().lv2_features().lv2_features(&info->world(), this);

	// Actually create plugin instances and port buffers.
	const SampleRate rate = bufs.engine().driver()->sample_rate();
	_instances = new Instances(_polyphony, SharedPtr<void>());
	for (uint32_t i = 0; i < _polyphony; ++i) {
		_instances->at(i) = make_instance(bufs.uris(), rate, i, false);
		if (!_instances->at(i)) {
			return false;
		}
	}

	// FIXME: Polyphony + worker?
	if (lilv_plugin_has_feature(plug, info->work_schedule)) {
		_worker_iface = (const LV2_Worker_Interface*)
			lilv_instance_get_extension_data(instance(0),
			                                 LV2_WORKER__interface);
	}

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

LV2_Worker_Status
LV2Node::work_respond(LV2_Worker_Respond_Handle handle,
                      uint32_t                  size,
                      const void*               data)
{
	LV2Node* node = (LV2Node*)handle;
	LV2Node::Response* r = new LV2Node::Response(size, data);
	node->_responses.push_back(*r);
	return LV2_WORKER_SUCCESS;
}

void
LV2Node::work(uint32_t size, const void* data)
{
	if (_worker_iface) {
		LV2_Handle inst = lilv_instance_get_handle(instance(0));
		if (_worker_iface->work(inst, work_respond, this, size, data)) {
			Raul::error(Raul::fmt("Error calling %1% work method\n") % _path);
		}
	}
}

void
LV2Node::process(ProcessContext& context)
{
	NodeImpl::pre_process(context);

	for (uint32_t i = 0; i < _polyphony; ++i)
		lilv_instance_run(instance(i), context.nframes());

	if (_worker_iface) {
		LV2_Handle inst = lilv_instance_get_handle(instance(0));
		while (!_responses.empty()) {
			Response& r = _responses.front();
			_worker_iface->work_response(inst, r.size, r.data);
			_responses.pop_front();
			context.engine().maid()->push(&r);
		}

		if (_worker_iface->end_run) {
			_worker_iface->end_run(inst);
		}
	}

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

