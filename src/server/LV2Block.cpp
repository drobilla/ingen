/*
  This file is part of Ingen.
  Copyright 2007-2013 David Robillard <http://drobilla.net/>

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

#include "lv2/lv2plug.in/ns/ext/morph/morph.h"
#include "lv2/lv2plug.in/ns/ext/presets/presets.h"
#include "lv2/lv2plug.in/ns/ext/options/options.h"
#include "lv2/lv2plug.in/ns/ext/resize-port/resize-port.h"
#include "lv2/lv2plug.in/ns/ext/state/state.h"

#include "raul/Maid.hpp"
#include "raul/Array.hpp"

#include "ingen/Log.hpp"
#include "ingen/URIMap.hpp"
#include "ingen/URIs.hpp"

#include "Buffer.hpp"
#include "Driver.hpp"
#include "Engine.hpp"
#include "GraphImpl.hpp"
#include "InputPort.hpp"
#include "LV2Block.hpp"
#include "LV2Plugin.hpp"
#include "OutputPort.hpp"
#include "ProcessContext.hpp"

using namespace std;

namespace Ingen {
namespace Server {

/** Partially construct a LV2Block.
 *
 * Object is not usable until instantiate() is called with success.
 * (It _will_ crash!)
 */
LV2Block::LV2Block(LV2Plugin*          plugin,
                   const Raul::Symbol& symbol,
                   bool                polyphonic,
                   GraphImpl*          parent,
                   SampleRate          srate)
	: BlockImpl(plugin, symbol, polyphonic, parent, srate)
	, _lv2_plugin(plugin)
	, _instances(NULL)
	, _prepared_instances(NULL)
	, _worker_iface(NULL)
{
	assert(_lv2_plugin);
}

LV2Block::~LV2Block()
{
	delete _instances;
}

SPtr<LilvInstance>
LV2Block::make_instance(URIs&      uris,
                        SampleRate rate,
                        uint32_t   voice,
                        bool       preparing)
{
	LilvWorld*        lworld = _lv2_plugin->lv2_info()->lv2_world();
	const LilvPlugin* lplug  = _lv2_plugin->lilv_plugin();
	LilvInstance*     inst   = lilv_plugin_instantiate(
		lplug, rate, _features->array());

	if (!inst) {
		parent_graph()->engine().log().error(
			fmt("Failed to instantiate <%1%>\n")
			% _lv2_plugin->uri().c_str());
		return SPtr<LilvInstance>();
	}

	LilvNode* opt_interface = lilv_new_uri(lworld, LV2_OPTIONS__interface);

	const LV2_Options_Interface* options_iface = NULL;
	if (lilv_plugin_has_extension_data(lplug, opt_interface)) {
		options_iface = (const LV2_Options_Interface*)
			lilv_instance_get_extension_data(inst, LV2_OPTIONS__interface);
	}

	lilv_node_free(opt_interface);

	for (uint32_t p = 0; p < num_ports(); ++p) {
		PortImpl* const port   = _ports->at(p);
		Buffer* const   buffer = (preparing)
			? port->prepared_buffer(voice).get()
			: port->buffer(voice).get();
		if (port->is_morph() && port->is_a(PortType::CV)) {
			if (options_iface) {
				const LV2_URID port_type = uris.lv2_CVPort;
				const LV2_Options_Option options[] = {
					{ LV2_OPTIONS_PORT, p, uris.morph_currentType,
					  sizeof(LV2_URID), uris.atom_URID, &port_type },
					{ LV2_OPTIONS_INSTANCE, 0, 0, 0, 0, NULL }
				};
				options_iface->set(inst->lv2_handle, options);
			}
		}

		if (buffer) {
			if (port->is_a(PortType::CV) || port->is_a(PortType::CONTROL)) {
				buffer->set_block(port->value().get<float>(), 0, buffer->nframes());
			} else {
				buffer->clear();
			}
		}
	}

	if (options_iface) {
		for (uint32_t p = 0; p < num_ports(); ++p) {
			PortImpl* const port = _ports->at(p);
			if (port->is_auto_morph()) {
				LV2_Options_Option options[] = {
					{ LV2_OPTIONS_PORT, p, uris.morph_currentType, 0, 0, NULL },
					{ LV2_OPTIONS_INSTANCE, 0, 0, 0, 0, 0 }
				};

				options_iface->get(inst->lv2_handle, options);
				if (options[0].value) {
					LV2_URID type = *(const LV2_URID*)options[0].value;
					if (type == _uris.lv2_ControlPort) {
						port->set_type(PortType::CONTROL, 0);
					} else if (type == _uris.lv2_CVPort) {
						port->set_type(PortType::CV, 0);
					} else {
						parent_graph()->engine().log().error(
							fmt("%1% auto-morphed to unknown type %2%\n")
							% port->path().c_str() % type);
						return SPtr<LilvInstance>();
					}
				} else {
					parent_graph()->engine().log().error(
						fmt("Failed to get auto-morphed type of %1%\n")
						% port->path().c_str());
				}
			}
		}
	}

	return SPtr<LilvInstance>(inst, lilv_instance_free);
}

bool
LV2Block::prepare_poly(BufferFactory& bufs, uint32_t poly)
{
	if (!_polyphonic)
		poly = 1;

	BlockImpl::prepare_poly(bufs, poly);

	if (_polyphony == poly)
		return true;

	const SampleRate rate = bufs.engine().driver()->sample_rate();
	assert(!_prepared_instances);
	_prepared_instances = new Instances(poly, *_instances, SPtr<void>());
	for (uint32_t i = _polyphony; i < _prepared_instances->size(); ++i) {
		SPtr<LilvInstance> inst = make_instance(bufs.uris(), rate, i, true);
		if (!inst) {
			delete _prepared_instances;
			_prepared_instances = NULL;
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
LV2Block::apply_poly(ProcessContext& context, Raul::Maid& maid, uint32_t poly)
{
	if (!_polyphonic)
		poly = 1;

	if (_prepared_instances) {
		maid.dispose(_instances);
		_instances = _prepared_instances;
		_prepared_instances = NULL;
	}
	assert(poly <= _instances->size());

	return BlockImpl::apply_poly(context, maid, poly);
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
LV2Block::instantiate(BufferFactory& bufs)
{
	const Ingen::URIs& uris      = bufs.uris();
	SPtr<LV2Info>      info      = _lv2_plugin->lv2_info();
	const LilvPlugin*  plug      = _lv2_plugin->lilv_plugin();
	Ingen::Forge&      forge     = bufs.forge();
	const uint32_t     num_ports = lilv_plugin_get_num_ports(plug);

	LilvNode* lv2_connectionOptional = lilv_new_uri(
		bufs.engine().world()->lilv_world(), LV2_CORE__connectionOptional);

	_ports = new Raul::Array<PortImpl*>(num_ports, NULL);

	bool ret = true;

	float* min_values = new float[num_ports];
	float* max_values = new float[num_ports];
	float* def_values = new float[num_ports];
	lilv_plugin_get_port_ranges_float(plug, min_values, max_values, def_values);
	uint32_t max_sequence_size = 0;

	// Get all the necessary information about ports
	for (uint32_t j = 0; j < num_ports; ++j) {
		const LilvPort* id = lilv_plugin_get_port_by_index(plug, j);

		/* LV2 port symbols are guaranteed to be unique, valid C identifiers,
		   and Lilv guarantees that lilv_port_get_symbol() is valid. */
		const Raul::Symbol port_sym(
			lilv_node_as_string(lilv_port_get_symbol(plug, id)));

		// Get port type
		Atom     val;
		PortType port_type     = PortType::UNKNOWN;
		LV2_URID buffer_type   = 0;
		bool     is_morph      = false;
		bool     is_auto_morph = false;
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

		const bool optional = lilv_port_has_property(
			plug, id, lv2_connectionOptional);

		uint32_t port_buffer_size = bufs.default_size(buffer_type);
		if (port_buffer_size == 0 && !optional) {
			parent_graph()->engine().log().error(
				fmt("<%1%> port `%2%' has unknown buffer type\n")
				% _lv2_plugin->uri().c_str() % port_sym.c_str());
			ret = false;
			break;
		}

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
				} else if (lilv_node_is_uri(d)) {
					const char* uri_val = lilv_node_as_uri(d);
					val = forge.make_urid(
						bufs.engine().world()->uri_map().map_uri(uri_val));
				}
			}
			lilv_nodes_free(defaults);

			if (!val.type() && buffer_type == _uris.atom_URID) {
				val = forge.make_urid(0);
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
			lilv_nodes_free(sizes);
			max_sequence_size = std::max(port_buffer_size, max_sequence_size);
			bufs.set_seq_size(max_sequence_size);
		}

		enum { UNKNOWN, INPUT, OUTPUT } direction = UNKNOWN;
		if (lilv_port_is_a(plug, id, info->lv2_InputPort)) {
			direction = INPUT;
		} else if (lilv_port_is_a(plug, id, info->lv2_OutputPort)) {
			direction = OUTPUT;
		}

		if ((port_type == PortType::UNKNOWN && !optional) ||
		    direction == UNKNOWN) {
			parent_graph()->engine().log().error(
				fmt("<%1%> port `%2%' has unknown type or direction\n")
				% _lv2_plugin->uri().c_str() % port_sym.c_str());
			ret = false;
			break;
		}

		if (!val.type() && (port_type != PortType::ATOM)) {
			// Ensure numeric ports have a value, use 0 by default
			val = forge.make(isnan(def_values[j]) ? 0.0f : def_values[j]);
		}

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
				port->set_minimum(forge.make(min_values[j]));
			}
			if (!isnan(max_values[j])) {
				port->set_maximum(forge.make(max_values[j]));
			}
		}

		// Inherit certain properties from plugin port
		const LilvNode* preds[] = { info->lv2_designation,
		                            info->lv2_portProperty,
		                            info->atom_supports,
		                            0 };
		for (int p = 0; preds[p]; ++p) {
			LilvNodes* values = lilv_port_get_value(plug, id, preds[p]);
			LILV_FOREACH(nodes, v, values) {
				const LilvNode* val = lilv_nodes_get(values, v);
				if (lilv_node_is_uri(val)) {
					port->add_property(Raul::URI(lilv_node_as_uri(preds[p])),
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

	lilv_node_free(lv2_connectionOptional);

	if (!ret) {
		delete _ports;
		_ports = NULL;
		return ret;
	}

	_features = info->world().lv2_features().lv2_features(&info->world(), this);

	// Actually create plugin instances and port buffers.
	const SampleRate rate = bufs.engine().driver()->sample_rate();
	_instances = new Instances(_polyphony, SPtr<void>());
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

BlockImpl*
LV2Block::duplicate(Engine&             engine,
                    const Raul::Symbol& symbol,
                    GraphImpl*          parent)
{
	const SampleRate rate = engine.driver()->sample_rate();

	// Duplicate and instantiate block
	LV2Block* dup = new LV2Block(_lv2_plugin, symbol, _polyphonic, parent, rate);
	if (!dup->instantiate(*engine.buffer_factory())) {
		delete dup;
		return NULL;
	}
	dup->set_properties(properties());

	// Set duplicate port values and properties to the same as ours
	for (uint32_t p = 0; p < num_ports(); ++p) {
		const Atom& val = port_impl(p)->value();
		if (val.is_valid()) {
			dup->port_impl(p)->set_value(val);
		}
		dup->port_impl(p)->set_properties(port_impl(p)->properties());
	}

	// Copy internal plugin state
	for (uint32_t v = 0; v < _polyphony; ++v) {
		LilvState* state = lilv_state_new_from_instance(
			_lv2_plugin->lilv_plugin(), instance(v),
			&engine.world()->uri_map().urid_map_feature()->urid_map,
			NULL, NULL, NULL, NULL, NULL, NULL, LV2_STATE_IS_NATIVE, NULL);
		if (state) {
			lilv_state_restore(state, dup->instance(v),
			                   NULL, NULL, LV2_STATE_IS_NATIVE, NULL);
			lilv_state_free(state);
		}
	}

	return dup;
}

void
LV2Block::activate(BufferFactory& bufs)
{
	BlockImpl::activate(bufs);

	for (uint32_t i = 0; i < _polyphony; ++i)
		lilv_instance_activate(instance(i));
}

void
LV2Block::deactivate()
{
	BlockImpl::deactivate();

	for (uint32_t i = 0; i < _polyphony; ++i)
		lilv_instance_deactivate(instance(i));
}

LV2_Worker_Status
LV2Block::work_respond(LV2_Worker_Respond_Handle handle,
                       uint32_t                  size,
                       const void*               data)
{
	LV2Block* block = (LV2Block*)handle;
	LV2Block::Response* r = new LV2Block::Response(size, data);
	block->_responses.push_back(*r);
	return LV2_WORKER_SUCCESS;
}

void
LV2Block::work(uint32_t size, const void* data)
{
	if (_worker_iface) {
		LV2_Handle inst = lilv_instance_get_handle(instance(0));
		if (_worker_iface->work(inst, work_respond, this, size, data)) {
			parent_graph()->engine().log().error(
				fmt("Error calling %1% work method\n") % _path);
		}
	}
}

void
LV2Block::run(ProcessContext& context)
{
	for (uint32_t i = 0; i < _polyphony; ++i)
		lilv_instance_run(instance(i), context.nframes());
}

void
LV2Block::post_process(ProcessContext& context)
{
	BlockImpl::post_process(context);

	if (_worker_iface) {
		LV2_Handle inst = lilv_instance_get_handle(instance(0));
		while (!_responses.empty()) {
			Response& r = _responses.front();
			_worker_iface->work_response(inst, r.size, r.data);
			_responses.pop_front();
			context.engine().maid()->dispose(&r);
		}

		if (_worker_iface->end_run) {
			_worker_iface->end_run(inst);
		}
	}
}

LilvState*
LV2Block::load_preset(const Raul::URI& uri)
{
	World*     world  = &_lv2_plugin->lv2_info()->world();
	LilvWorld* lworld = _lv2_plugin->lv2_info()->lv2_world();
	LilvNode*  preset = lilv_new_uri(lworld, uri.c_str());

	// Load preset into world if necessary
	lilv_world_load_resource(lworld, preset);

	// Load preset from world
	LV2_URID_Map* map   = &world->uri_map().urid_map_feature()->urid_map;
	LilvState*    state = lilv_state_new_from_world(lworld, map, preset);

	lilv_node_free(preset);
	return state;
}

void
LV2Block::apply_state(LilvState* state)
{
	for (uint32_t v = 0; v < _polyphony; ++v) {
		lilv_state_restore(state, instance(v), NULL, NULL, 0, NULL);
	}
}

void
LV2Block::set_port_buffer(uint32_t    voice,
                          uint32_t    port_num,
                          BufferRef   buf,
                          SampleCount offset)
{
	BlockImpl::set_port_buffer(voice, port_num, buf, offset);
	lilv_instance_connect_port(
		instance(voice),
		port_num,
		buf ? buf->port_data(_ports->at(port_num)->type(), offset) : NULL);
}

} // namespace Server
} // namespace Ingen
