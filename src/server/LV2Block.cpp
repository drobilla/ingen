/*
  This file is part of Ingen.
  Copyright 2007-2016 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Buffer.hpp"
#include "Engine.hpp"
#include "GraphImpl.hpp"
#include "InputPort.hpp"
#include "LV2Block.hpp"
#include "LV2Plugin.hpp"
#include "OutputPort.hpp"
#include "PortImpl.hpp"
#include "RunContext.hpp"
#include "Worker.hpp"

#include "ingen/FilePath.hpp"
#include "ingen/Forge.hpp"
#include "ingen/Log.hpp"
#include "ingen/URI.hpp"
#include "ingen/URIMap.hpp"
#include "ingen/URIs.hpp"
#include "ingen/World.hpp"
#include "lv2/options/options.h"
#include "lv2/state/state.h"
#include "raul/Array.hpp"
#include "raul/Maid.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

namespace ingen {
namespace server {

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
	, _worker_iface(nullptr)
{
	assert(_lv2_plugin);
}

LV2Block::~LV2Block()
{
	if (_activated) {
		LV2Block::deactivate();
	}

	// Explicitly drop instances first to prevent reference cycles
	drop_instances(_instances);
	drop_instances(_prepared_instances);
}

std::shared_ptr<LV2Block::Instance>
LV2Block::make_instance(URIs&      uris,
                        SampleRate rate,
                        uint32_t   voice,
                        bool       preparing)
{
	const Engine&     engine = parent_graph()->engine();
	const LilvPlugin* lplug  = _lv2_plugin->lilv_plugin();
	LilvInstance*     inst   = lilv_plugin_instantiate(
		lplug, rate, _features->array());

	if (!inst) {
		engine.log().error("Failed to instantiate <%1%>\n",
		                   _lv2_plugin->uri().c_str());
		return nullptr;
	}

	const LV2_Options_Interface* options_iface = nullptr;
	if (lilv_plugin_has_extension_data(lplug, uris.opt_interface)) {
		options_iface = static_cast<const LV2_Options_Interface*>(
			lilv_instance_get_extension_data(inst, LV2_OPTIONS__interface));
	}

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
					{ LV2_OPTIONS_INSTANCE, 0, 0, 0, 0, nullptr }
				};
				options_iface->set(inst->lv2_handle, options);
			}
		}

		if (buffer) {
			if (port->is_a(PortType::CONTROL)) {
				buffer->set_value(port->value());
			} else if (port->is_a(PortType::CV)) {
				buffer->set_block(port->value().get<float>(), 0, engine.block_length());
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
					{ LV2_OPTIONS_PORT, p, uris.morph_currentType, 0, 0, nullptr },
					{ LV2_OPTIONS_INSTANCE, 0, 0, 0, 0, nullptr }
				};

				options_iface->get(inst->lv2_handle, options);
				if (options[0].value) {
					LV2_URID type = *static_cast<const LV2_URID*>(options[0].value);
					if (type == _uris.lv2_ControlPort) {
						port->set_type(PortType::CONTROL, 0);
					} else if (type == _uris.lv2_CVPort) {
						port->set_type(PortType::CV, 0);
					} else {
						parent_graph()->engine().log().error(
							"%1% auto-morphed to unknown type %2%\n",
							port->path().c_str(),
							type);
						return nullptr;
					}
				} else {
					parent_graph()->engine().log().error(
						"Failed to get auto-morphed type of %1%\n",
						port->path().c_str());
				}
			}
		}
	}

	return std::make_shared<Instance>(inst);
}

bool
LV2Block::prepare_poly(BufferFactory& bufs, uint32_t poly)
{
	if (!_polyphonic) {
		poly = 1;
	}

	BlockImpl::prepare_poly(bufs, poly);

	if (_polyphony == poly) {
		return true;
	}

	const SampleRate rate = bufs.engine().sample_rate();
	assert(!_prepared_instances);
	_prepared_instances = bufs.maid().make_managed<Instances>(
		poly, *_instances, nullptr);
	for (uint32_t i = _polyphony; i < _prepared_instances->size(); ++i) {
		auto inst = make_instance(bufs.uris(), rate, i, true);
		if (!inst) {
			_prepared_instances.reset();
			return false;
		}

		_prepared_instances->at(i) = inst;

		if (_activated) {
			lilv_instance_activate(inst->instance);
		}
	}

	return true;
}

bool
LV2Block::apply_poly(RunContext& ctx, uint32_t poly)
{
	if (!_polyphonic) {
		poly = 1;
	}

	if (_prepared_instances) {
		_instances = std::move(_prepared_instances);
	}
	assert(poly <= _instances->size());

	return BlockImpl::apply_poly(ctx, poly);
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
LV2Block::instantiate(BufferFactory& bufs, const LilvState* state)
{
	const ingen::URIs& uris      = bufs.uris();
	ingen::World&      world     = bufs.engine().world();
	const LilvPlugin*  plug      = _lv2_plugin->lilv_plugin();
	ingen::Forge&      forge     = bufs.forge();
	const uint32_t     num_ports = lilv_plugin_get_num_ports(plug);

	LilvNode* lv2_connectionOptional = lilv_new_uri(
		world.lilv_world(), LV2_CORE__connectionOptional);

	_ports = bufs.maid().make_managed<BlockImpl::Ports>(num_ports, nullptr);

	bool ret = true;

	auto* min_values = new float[num_ports];
	auto* max_values = new float[num_ports];
	auto* def_values = new float[num_ports];
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
		if (lilv_port_is_a(plug, id, uris.lv2_ControlPort)) {
			if (lilv_port_is_a(plug, id, uris.morph_MorphPort)) {
				is_morph = true;
				LilvNodes* types = lilv_port_get_value(
					plug, id, uris.morph_supportsType);
				LILV_FOREACH(nodes, i, types) {
					const LilvNode* type = lilv_nodes_get(types, i);
					if (lilv_node_equals(type, uris.lv2_CVPort)) {
						port_type   = PortType::CV;
						buffer_type = uris.atom_Sound;
					}
				}
				lilv_nodes_free(types);
			}
			if (port_type == PortType::UNKNOWN) {
				port_type   = PortType::CONTROL;
				buffer_type = uris.atom_Sequence;
			}
		} else if (lilv_port_is_a(plug, id, uris.lv2_CVPort)) {
			port_type   = PortType::CV;
			buffer_type = uris.atom_Sound;
		} else if (lilv_port_is_a(plug, id, uris.lv2_AudioPort)) {
			port_type   = PortType::AUDIO;
			buffer_type = uris.atom_Sound;
		} else if (lilv_port_is_a(plug, id, uris.atom_AtomPort)) {
			port_type = PortType::ATOM;
		}

		if (lilv_port_is_a(plug, id, uris.morph_AutoMorphPort)) {
			is_auto_morph = true;
		}

		// Get buffer type if necessary (atom ports)
		if (!buffer_type) {
			LilvNodes* types = lilv_port_get_value(
				plug, id, uris.atom_bufferType);
			LILV_FOREACH(nodes, i, types) {
				const LilvNode* type = lilv_nodes_get(types, i);
				if (lilv_node_is_uri(type)) {
					buffer_type = world.uri_map().map_uri(
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
				"<%1%> port `%2%' has unknown buffer type\n",
				_lv2_plugin->uri().c_str(), port_sym.c_str());
			ret = false;
			break;
		}

		if (port_type == PortType::ATOM) {
			// Get default value, and its length
			LilvNodes* defaults = lilv_port_get_value(plug, id, uris.lv2_default);
			LILV_FOREACH(nodes, i, defaults) {
				const LilvNode* d = lilv_nodes_get(defaults, i);
				if (lilv_node_is_string(d)) {
					const char*    str_val     = lilv_node_as_string(d);
					const uint32_t str_val_len = strlen(str_val);
					val = forge.alloc(str_val);
					port_buffer_size = std::max(port_buffer_size, str_val_len);
				} else if (lilv_node_is_uri(d)) {
					const char* uri_val = lilv_node_as_uri(d);
					val = forge.make_urid(world.uri_map().map_uri(uri_val));
				}
			}
			lilv_nodes_free(defaults);

			if (!val.type() && buffer_type == _uris.atom_URID) {
				val = forge.make_urid(0);
			}

			// Get minimum size, if set in data
			LilvNodes* sizes = lilv_port_get_value(plug, id, uris.rsz_minimumSize);
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
		if (lilv_port_is_a(plug, id, uris.lv2_InputPort)) {
			direction = INPUT;
		} else if (lilv_port_is_a(plug, id, uris.lv2_OutputPort)) {
			direction = OUTPUT;
		}

		if ((port_type == PortType::UNKNOWN && !optional) ||
		    direction == UNKNOWN) {
			parent_graph()->engine().log().error(
				"<%1%> port `%2%' has unknown type or direction\n",
				_lv2_plugin->uri().c_str(), port_sym.c_str());
			ret = false;
			break;
		}

		if (!val.type() && (port_type != PortType::ATOM)) {
			// Ensure numeric ports have a value
			if (!std::isnan(def_values[j])) {
				val = forge.make(def_values[j]);
			} else if (!std::isnan(min_values[j])) {
				val = forge.make(min_values[j]);
			} else if (!std::isnan(max_values[j])) {
				val = forge.make(max_values[j]);
			} else {
				val = forge.make(0.0f);
			}
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
			if (!std::isnan(min_values[j])) {
				port->set_minimum(forge.make(min_values[j]));
			}
			if (!std::isnan(max_values[j])) {
				port->set_maximum(forge.make(max_values[j]));
			}
		}

		// Inherit certain properties from plugin port
		const LilvNode* preds[] = { uris.lv2_designation,
		                            uris.lv2_portProperty,
		                            uris.atom_supports,
		                            nullptr };
		for (int p = 0; preds[p]; ++p) {
			LilvNodes* values = lilv_port_get_value(plug, id, preds[p]);
			LILV_FOREACH(nodes, v, values) {
				const LilvNode* value = lilv_nodes_get(values, v);
				if (lilv_node_is_uri(value)) {
					port->add_property(URI(lilv_node_as_uri(preds[p])),
					                   forge.make_urid(URI(lilv_node_as_uri(value))));
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
		_ports.reset();
		return ret;
	}

	_features = world.lv2_features().lv2_features(world, this);

	// Actually create plugin instances and port buffers.
	const SampleRate rate = bufs.engine().sample_rate();
	_instances = bufs.maid().make_managed<Instances>(
		_polyphony, nullptr);
	for (uint32_t i = 0; i < _polyphony; ++i) {
		_instances->at(i) = make_instance(bufs.uris(), rate, i, false);
		if (!_instances->at(i)) {
			return false;
		}
	}

	// Load initial state if no state is explicitly given
	LilvState* default_state = nullptr;
	if (!state) {
		state = default_state = load_preset(_lv2_plugin->uri());
	}

	// Apply state
	if (state) {
		apply_state(nullptr, state);
	}

	if (default_state) {
		lilv_state_free(default_state);
	}

	// FIXME: Polyphony + worker?
	if (lilv_plugin_has_feature(plug, uris.work_schedule)) {
		_worker_iface = static_cast<const LV2_Worker_Interface*>(
			lilv_instance_get_extension_data(instance(0),
			                                 LV2_WORKER__interface));
	}

	return ret;
}

bool
LV2Block::save_state(const FilePath& dir) const
{
	World&     world  = _lv2_plugin->world();
	LilvWorld* lworld = world.lilv_world();

	LilvState* state = lilv_state_new_from_instance(
		_lv2_plugin->lilv_plugin(), const_cast<LV2Block*>(this)->instance(0),
		&world.uri_map().urid_map(),
		nullptr, dir.c_str(), dir.c_str(), dir.c_str(), nullptr, nullptr,
		LV2_STATE_IS_POD|LV2_STATE_IS_PORTABLE, nullptr);

	if (!state) {
		return false;
	} else if (lilv_state_get_num_properties(state) == 0) {
		lilv_state_free(state);
		return false;
	}

	lilv_state_save(lworld,
	                &world.uri_map().urid_map(),
	                &world.uri_map().urid_unmap(),
	                state,
	                nullptr,
	                dir.c_str(),
	                "state.ttl");

	lilv_state_free(state);

	return true;
}

BlockImpl*
LV2Block::duplicate(Engine&             engine,
                    const Raul::Symbol& symbol,
                    GraphImpl*          parent)
{
	const SampleRate rate = engine.sample_rate();

	// Get current state
	LilvState* state = lilv_state_new_from_instance(
		_lv2_plugin->lilv_plugin(), instance(0),
		&engine.world().uri_map().urid_map(),
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, LV2_STATE_IS_NATIVE, nullptr);

	// Duplicate and instantiate block
	auto* dup = new LV2Block(_lv2_plugin, symbol, _polyphonic, parent, rate);
	if (!dup->instantiate(*engine.buffer_factory(), state)) {
		delete dup;
		return nullptr;
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

	return dup;
}

void
LV2Block::activate(BufferFactory& bufs)
{
	BlockImpl::activate(bufs);

	for (uint32_t i = 0; i < _polyphony; ++i) {
		lilv_instance_activate(instance(i));
	}
}

void
LV2Block::deactivate()
{
	BlockImpl::deactivate();

	for (uint32_t i = 0; i < _polyphony; ++i) {
		lilv_instance_deactivate(instance(i));
	}
}

LV2_Worker_Status
LV2Block::work_respond(LV2_Worker_Respond_Handle handle,
                       uint32_t                  size,
                       const void*               data)
{
	auto* block = static_cast<LV2Block*>(handle);
	auto* r     = new LV2Block::Response(size, data);
	block->_responses.push_back(*r);
	return LV2_WORKER_SUCCESS;
}

LV2_Worker_Status
LV2Block::work(uint32_t size, const void* data)
{
	if (_worker_iface) {
		std::lock_guard<std::mutex> lock(_work_mutex);

		LV2_Handle        inst = lilv_instance_get_handle(instance(0));
		LV2_Worker_Status st   = _worker_iface->work(inst, work_respond, this, size, data);
		if (st) {
			parent_graph()->engine().log().error(
				"Error calling %1% work method\n", _path);
		}
		return st;
	}
	return LV2_WORKER_ERR_UNKNOWN;
}

void
LV2Block::run(RunContext& ctx)
{
	for (uint32_t i = 0; i < _polyphony; ++i) {
		lilv_instance_run(instance(i), ctx.nframes());
	}
}

void
LV2Block::post_process(RunContext& ctx)
{
	/* Handle any worker responses.  Note that this may write to output ports,
	   so must be done first to prevent clobbering worker responses and
	   monitored notification ports. */
	if (_worker_iface) {
		LV2_Handle inst = lilv_instance_get_handle(instance(0));
		while (!_responses.empty()) {
			Response& r = _responses.front();
			_worker_iface->work_response(inst, r.size, r.data);
			_responses.pop_front();
			ctx.engine().maid()->dispose(&r);
		}

		if (_worker_iface->end_run) {
			_worker_iface->end_run(inst);
		}
	}

	/* Run cycle truly finished, finalise output ports. */
	BlockImpl::post_process(ctx);
}

LilvState*
LV2Block::load_preset(const URI& uri)
{
	World&     world  = _lv2_plugin->world();
	LilvWorld* lworld = world.lilv_world();
	LilvNode*  preset = lilv_new_uri(lworld, uri.c_str());

	// Load preset into world if necessary
	lilv_world_load_resource(lworld, preset);

	// Load preset from world
	LV2_URID_Map* map   = &world.uri_map().urid_map();
	LilvState*    state = lilv_state_new_from_world(lworld, map, preset);

	lilv_node_free(preset);
	return state;
}

LilvState*
LV2Block::load_state(World& world, const FilePath& path)
{
	LilvWorld* lworld  = world.lilv_world();
	const URI  uri     = URI(path);
	LilvNode*  subject = lilv_new_uri(lworld, uri.c_str());

	LilvState* state = lilv_state_new_from_file(
		lworld,
		&world.uri_map().urid_map(),
		subject,
		path.c_str());

	lilv_node_free(subject);
	return state;
}

void
LV2Block::apply_state(const std::unique_ptr<Worker>& worker,
                      const LilvState*               state)
{
	World&                       world = parent_graph()->engine().world();
	std::shared_ptr<LV2_Feature> sched;
	if (worker) {
		sched = worker->schedule_feature()->feature(world, this);
	}

	const LV2_Feature* state_features[2] = { nullptr, nullptr };
	if (sched) {
		state_features[0] = sched.get();
	}

	for (uint32_t v = 0; v < _polyphony; ++v) {
		lilv_state_restore(state, instance(v), nullptr, nullptr, 0, state_features);
	}
}

static const void*
get_port_value(const char* port_symbol,
               void*       user_data,
               uint32_t*   size,
               uint32_t*   type)
{
	auto* const block = static_cast<LV2Block*>(user_data);
	auto* const port  = block->port_by_symbol(port_symbol);

	if (port && port->is_input() && port->value().is_valid()) {
		*size = port->value().size();
		*type = port->value().type();
		return port->value().get_body();
	}

	return nullptr;
}

boost::optional<Resource>
LV2Block::save_preset(const URI&        uri,
                      const Properties& props)
{
	World&          world  = parent_graph()->engine().world();
	LilvWorld*      lworld = world.lilv_world();
	LV2_URID_Map*   lmap   = &world.uri_map().urid_map();
	LV2_URID_Unmap* lunmap = &world.uri_map().urid_unmap();

	const FilePath path     = FilePath(uri.path());
	const FilePath dirname  = path.parent_path();
	const FilePath basename = path.stem();

	LilvState* state = lilv_state_new_from_instance(
		_lv2_plugin->lilv_plugin(), instance(0), lmap,
		nullptr, nullptr, nullptr, path.c_str(),
		get_port_value, this, LV2_STATE_IS_NATIVE, nullptr);

	if (state) {
		const Properties::const_iterator l = props.find(_uris.rdfs_label);
		if (l != props.end() && l->second.type() == _uris.atom_String) {
			lilv_state_set_label(state, l->second.ptr<char>());
		}

		lilv_state_save(lworld, lmap, lunmap, state, nullptr,
		                dirname.c_str(), basename.c_str());

		const URI         state_uri(lilv_node_as_uri(lilv_state_get_uri(state)));
		const std::string label(lilv_state_get_label(state)
		                        ? lilv_state_get_label(state)
		                        : basename);
		lilv_state_free(state);

		Resource preset(_uris, state_uri);
		preset.set_property(_uris.rdf_type, _uris.pset_Preset);
		preset.set_property(_uris.rdfs_label, world.forge().alloc(label));
		preset.set_property(_uris.lv2_appliesTo,
		                    world.forge().make_urid(_lv2_plugin->uri()));

		const std::string bundle_uri = URI(dirname).string() + '/';
		LilvNode* lbundle = lilv_new_uri(lworld, bundle_uri.c_str());
		lilv_world_load_bundle(lworld, lbundle);
		lilv_node_free(lbundle);

		return {preset};
	}

	return boost::optional<Resource>();
}

void
LV2Block::set_port_buffer(uint32_t         voice,
                          uint32_t         port_num,
                          const BufferRef& buf,
                          SampleCount      offset)
{
	BlockImpl::set_port_buffer(voice, port_num, buf, offset);
	lilv_instance_connect_port(
		instance(voice),
		port_num,
		buf ? buf->port_data(_ports->at(port_num)->type(), offset) : nullptr);
}

} // namespace server
} // namespace ingen
