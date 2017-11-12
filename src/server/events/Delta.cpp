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

#include <vector>
#include <thread>

#include <glibmm/convert.h>

#include "ingen/Log.hpp"
#include "ingen/Store.hpp"
#include "ingen/URIs.hpp"
#include "raul/Maid.hpp"

#include "Broadcaster.hpp"
#include "ControlBindings.hpp"
#include "CreateBlock.hpp"
#include "CreateGraph.hpp"
#include "CreatePort.hpp"
#include "Delta.hpp"
#include "Engine.hpp"
#include "GraphImpl.hpp"
#include "PluginImpl.hpp"
#include "PortImpl.hpp"
#include "PortType.hpp"
#include "SetPortValue.hpp"
#include "events/Get.hpp"

namespace Ingen {
namespace Server {
namespace Events {

Delta::Delta(Engine&           engine,
             SPtr<Interface>   client,
             int32_t           id,
             SampleCount       timestamp,
             Type              type,
             Resource::Graph   context,
             const Raul::URI&  subject,
             const Properties& properties,
             const Properties& remove)
	: Event(engine, client, id, timestamp)
	, _create_event(NULL)
	, _subject(subject)
	, _properties(properties)
	, _remove(remove)
	, _object(NULL)
	, _graph(NULL)
	, _binding(NULL)
	, _state(NULL)
	, _context(context)
	, _type(type)
	, _block(false)
{
	if (context != Resource::Graph::DEFAULT) {
		for (auto& p : _properties) {
			p.second.set_context(context);
		}
	}

	// Set atomic execution if polyphony is to be changed
	const Ingen::URIs& uris = _engine.world()->uris();
	if (properties.count(uris.ingen_polyphonic) ||
	    properties.count(uris.ingen_polyphony)) {
		_block = true;
	}
}

Delta::~Delta()
{
	for (auto& s : _set_events)
		delete s;

	delete _create_event;
}

void
Delta::add_set_event(const char* port_symbol,
                     const void* value,
                     uint32_t    size,
                     uint32_t    type)
{
	BlockImpl* block = dynamic_cast<BlockImpl*>(_object);
	PortImpl*  port  = block->port_by_symbol(port_symbol);
	if (!port) {
		_engine.log().warn(fmt("Unknown port `%1%' in state") % port_symbol);
		return;
	}

	SetPortValue* ev = new SetPortValue(
		_engine, _request_client, _request_id, _time,
		port, Atom(size, type, value), false, true);

	_set_events.push_back(ev);
}

static void
s_add_set_event(const char* port_symbol,
                void*       user_data,
                const void* value,
                uint32_t    size,
                uint32_t    type)
{
	((Delta*)user_data)->add_set_event(port_symbol, value, size, type);
}

static LilvNode*
get_file_node(LilvWorld* lworld, const URIs& uris, const Atom& value)
{
	if (value.type() == uris.atom_Path) {
		return lilv_new_file_uri(lworld, NULL, value.ptr<char>());
	} else if (uris.forge.is_uri(value)) {
		const std::string str = uris.forge.str(value, false);
		if (str.substr(0, 5) == "file:") {
			return lilv_new_uri(lworld, value.ptr<char>());
		}
	}
	return NULL;
}

bool
Delta::pre_process(PreProcessContext& ctx)
{
	const Ingen::URIs& uris = _engine.world()->uris();

	const bool is_graph_object = uri_is_path(_subject);
	const bool is_client       = (_subject == "ingen:/clients/this");
	const bool is_engine       = (_subject == "ingen:/");
	const bool is_file         = (_subject.substr(0, 5) == "file:");

	if (_type == Type::PUT && is_file) {
		// Ensure type is Preset, the only supported file put
		const auto t = _properties.find(uris.rdf_type);
		if (t == _properties.end() || t->second != uris.pset_Preset) {
			return Event::pre_process_done(Status::BAD_REQUEST, _subject);
		}

		// Get "prototype" for preset (node to save state for)
		const auto p = _properties.find(uris.lv2_prototype);
		if (p == _properties.end()) {
			return Event::pre_process_done(Status::BAD_REQUEST, _subject);
		} else if (!_engine.world()->forge().is_uri(p->second)) {
			return Event::pre_process_done(Status::BAD_REQUEST, _subject);
		}

		const Raul::URI prot(_engine.world()->forge().str(p->second, false));
		if (!uri_is_path(prot)) {
			return Event::pre_process_done(Status::BAD_URI, _subject);
		}

		Node* node = _engine.store()->get(uri_to_path(prot));
		if (!node) {
			return Event::pre_process_done(Status::NOT_FOUND, prot);
		}

		BlockImpl* block = dynamic_cast<BlockImpl*>(node);
		if (!block) {
			return Event::pre_process_done(Status::BAD_OBJECT_TYPE, prot);
		}

		if ((_preset = block->save_preset(_subject, _properties))) {
			return Event::pre_process_done(Status::SUCCESS);
		} else {
			return Event::pre_process_done(Status::FAILURE);
		}
	}

	std::lock_guard<Store::Mutex> lock(_engine.store()->mutex());

	_object = is_graph_object
		? static_cast<Ingen::Resource*>(_engine.store()->get(uri_to_path(_subject)))
		: static_cast<Ingen::Resource*>(_engine.block_factory()->plugin(_subject));

	if (!_object && !is_client && !is_engine &&
	    (!is_graph_object || _type != Type::PUT)) {
		return Event::pre_process_done(Status::NOT_FOUND, _subject);
	}

	if (is_graph_object && !_object) {
		Raul::Path path(uri_to_path(_subject));
		bool is_graph = false, is_block = false, is_port = false, is_output = false;
		Ingen::Resource::type(uris, _properties, is_graph, is_block, is_port, is_output);

		if (is_graph) {
			_create_event = new CreateGraph(
				_engine, _request_client, _request_id, _time, path, _properties);
		} else if (is_block) {
			_create_event = new CreateBlock(
				_engine, _request_client, _request_id, _time, path, _properties);
		} else if (is_port) {
			_create_event = new CreatePort(
				_engine, _request_client, _request_id, _time,
				path, _properties);
		}
		if (_create_event) {
			if (_create_event->pre_process(ctx)) {
				_object = _engine.store()->get(path);  // Get object for setting
			} else {
				return Event::pre_process_done(Status::CREATION_FAILED, _subject);
			}
		} else {
			return Event::pre_process_done(Status::BAD_OBJECT_TYPE, _subject);
		}
	}

	_types.reserve(_properties.size());

	NodeImpl* obj = dynamic_cast<NodeImpl*>(_object);

	// Remove any properties removed in delta
	for (const auto& r : _remove) {
		const Raul::URI& key   = r.first;
		const Atom&      value = r.second;
		if (key == uris.midi_binding && value == uris.patch_wildcard) {
			PortImpl* port = dynamic_cast<PortImpl*>(_object);
			if (port) {
				_engine.control_bindings()->get_all(port->path(), _removed_bindings);
			}
		}
		if (_object) {
			_removed.emplace(key, value);
			_object->remove_property(key, value);
		} else if (is_engine && key == uris.ingen_loadedBundle) {
 			LilvWorld* lworld = _engine.world()->lilv_world();
			LilvNode*  bundle = get_file_node(lworld, uris, value);
			if (bundle) {
				for (const auto& p : _engine.block_factory()->plugins()) {
					if (p.second->bundle_uri() == lilv_node_as_string(bundle)) {
						p.second->set_is_zombie(true);
						_update.del(p.second->uri());
					}
				}
				lilv_world_unload_bundle(lworld, bundle);
				_engine.block_factory()->refresh();
				lilv_node_free(bundle);
			} else {
				_status = Status::BAD_VALUE;
			}
		}
	}

	// Remove all added properties if this is a put or set
	if (_object && (_type == Type::PUT || _type == Type::SET)) {
		for (auto p = _properties.begin();
		     p != _properties.end();
		     p = _properties.upper_bound(p->first)) {
			for (auto q = _object->properties().find(p->first);
			     q != _object->properties().end() && q->first == p->first;) {
				auto next = q;
				++next;

				if (!_properties.contains(q->first, q->second)) {
					const auto r = std::make_pair(q->first, q->second);
					_object->properties().erase(q);
					_object->on_property_removed(r.first, r.second);
					_removed.insert(r);
				}

				q = next;
			}
		}
	}

	for (const auto& p : _properties) {
		const Raul::URI& key   = p.first;
		const Property&  value = p.second;
		SpecialType      op    = SpecialType::NONE;
		if (obj) {
			Resource& resource = *obj;
			if (value != uris.patch_wildcard) {
				if (resource.add_property(key, value, value.context())) {
					_added.emplace(key, value);
				}
			}

			BlockImpl* block = NULL;
			PortImpl*  port  = dynamic_cast<PortImpl*>(_object);
			if (port) {
				if (key == uris.ingen_broadcast) {
					if (value.type() == uris.forge.Bool) {
						op = SpecialType::ENABLE_BROADCAST;
					} else {
						_status = Status::BAD_VALUE_TYPE;
					}
				} else if (key == uris.ingen_value || key == uris.ingen_activity) {
					SetPortValue* ev = new SetPortValue(
						_engine, _request_client, _request_id, _time, port, value,
						key == uris.ingen_activity);
					_set_events.push_back(ev);
				} else if (key == uris.midi_binding) {
					if (port->is_a(PortType::CONTROL) || port->is_a(PortType::CV)) {
						if (value == uris.patch_wildcard) {
							_engine.control_bindings()->start_learn(port);
						} else if (value.type() == uris.atom_Object) {
							op       = SpecialType::CONTROL_BINDING;
							_binding = new ControlBindings::Binding();
						} else {
							_status = Status::BAD_VALUE_TYPE;
						}
					} else {
						_status = Status::BAD_OBJECT_TYPE;
					}
				}
			} else if ((block = dynamic_cast<BlockImpl*>(_object))) {
				if (key == uris.midi_binding && value == uris.patch_wildcard) {
					op = SpecialType::CONTROL_BINDING;  // Internal block learn
				} else if (key == uris.ingen_enabled) {
					if (value.type() == uris.forge.Bool) {
						op = SpecialType::ENABLE;
					} else {
						_status = Status::BAD_VALUE_TYPE;
					}
				} else if (key == uris.pset_preset) {
					std::string uri_str;
					if (uris.forge.is_uri(value)) {
						uri_str = uris.forge.str(value, false);
					} else if (value.type() == uris.forge.Path) {
						uri_str = Glib::filename_to_uri(value.ptr<char>());
					}

					if (Raul::URI::is_valid(uri_str)) {
						const Raul::URI uri(uri_str);
						op = SpecialType::PRESET;
						if ((_state = block->load_preset(uri))) {
							lilv_state_emit_port_values(
								_state, s_add_set_event, this);
						} else {
							_engine.log().warn(fmt("Failed to load preset <%1%>\n") % uri);
						}
					} else {
						_status = Status::BAD_VALUE;
					}
				}
			}

			if ((_graph = dynamic_cast<GraphImpl*>(_object))) {
				if (key == uris.ingen_enabled) {
					if (value.type() == uris.forge.Bool) {
						op = SpecialType::ENABLE;
						// FIXME: defer this until all other metadata has been processed
						if (value.get<int32_t>() && !_graph->enabled()) {
							if (!(_compiled_graph = compile(*_engine.maid(), *_graph))) {
								_status = Status::COMPILATION_FAILED;
							}
						}
					} else {
						_status = Status::BAD_VALUE_TYPE;
					}
				} else if (key == uris.ingen_polyphony) {
					if (value.type() == uris.forge.Int) {
						if (value.get<int32_t>() < 1 || value.get<int32_t>() > 128) {
							_status = Status::INVALID_POLY;
						} else {
							op = SpecialType::POLYPHONY;
							_graph->prepare_internal_poly(
								*_engine.buffer_factory(), value.get<int32_t>());
						}
					} else {
						_status = Status::BAD_VALUE_TYPE;
					}
				}
			}

			if (!_create_event && key == uris.ingen_polyphonic) {
				GraphImpl* parent = dynamic_cast<GraphImpl*>(obj->parent());
				if (!parent) {
					_status = Status::BAD_OBJECT_TYPE;
				} else if (value.type() != uris.forge.Bool) {
					_status = Status::BAD_VALUE_TYPE;
				} else {
					op     = SpecialType::POLYPHONIC;
					obj->set_property(key, value, value.context());
					BlockImpl* block = dynamic_cast<BlockImpl*>(obj);
					if (block) {
						block->set_polyphonic(value.get<int32_t>());
					}
					if (value.get<int32_t>()) {
						obj->prepare_poly(*_engine.buffer_factory(), parent->internal_poly());
					} else {
						obj->prepare_poly(*_engine.buffer_factory(), 1);
					}
				}
			}
		} else if (is_client && key == uris.ingen_broadcast) {
			_engine.broadcaster()->set_broadcast(
				_request_client, value.get<int32_t>());
		} else if (is_engine && key == uris.ingen_loadedBundle) {
 			LilvWorld* lworld = _engine.world()->lilv_world();
			LilvNode*  bundle = get_file_node(lworld, uris, value);
			if (bundle) {
				lilv_world_load_bundle(lworld, bundle);
				const std::set<PluginImpl*> new_plugins =
					_engine.block_factory()->refresh();

				for (PluginImpl* p : new_plugins) {
					if (p->bundle_uri() == lilv_node_as_string(bundle)) {
						_update.put_plugin(p);
					}
				}
				lilv_node_free(bundle);
			} else {
				_status = Status::BAD_VALUE;
			}
		}

		if (_status != Status::NOT_PREPARED) {
			break;
		}

		_types.push_back(op);
	}

	for (auto& s : _set_events) {
		s->pre_process(ctx);
	}

	return Event::pre_process_done(
		_status == Status::NOT_PREPARED ? Status::SUCCESS : _status,
		_subject);
}

void
Delta::execute(RunContext& context)
{
	if (_status != Status::SUCCESS || _preset) {
		return;
	}

	const Ingen::URIs& uris = _engine.world()->uris();

	if (_create_event) {
		_create_event->set_time(_time);
		_create_event->execute(context);
	}

	for (auto& s : _set_events) {
		s->set_time(_time);
		s->execute(context);
	}

	if (!_removed_bindings.empty()) {
		_engine.control_bindings()->remove(context, _removed_bindings);
	}

	NodeImpl* const  object = dynamic_cast<NodeImpl*>(_object);
	BlockImpl* const block  = dynamic_cast<BlockImpl*>(_object);
	PortImpl* const  port   = dynamic_cast<PortImpl*>(_object);

	std::vector<SpecialType>::const_iterator t = _types.begin();
	for (const auto& p : _properties) {
		const Raul::URI& key   = p.first;
		const Atom&      value = p.second;
		switch (*t++) {
		case SpecialType::ENABLE_BROADCAST:
			if (port) {
				port->enable_monitoring(value.get<int32_t>());
			}
			break;
		case SpecialType::ENABLE:
			if (_graph) {
				if (value.get<int32_t>()) {
					if (_compiled_graph) {
						_graph->set_compiled_graph(std::move(_compiled_graph));
					}
					_graph->enable();
				} else {
					_graph->disable(context);
				}
			} else if (block) {
				block->set_enabled(value.get<int32_t>());
			}
			break;
		case SpecialType::POLYPHONIC: {
			GraphImpl* parent = reinterpret_cast<GraphImpl*>(object->parent());
			if (value.get<int32_t>()) {
				object->apply_poly(context, parent->internal_poly_process());
			} else {
				object->apply_poly(context, 1);
			}
		} break;
		case SpecialType::POLYPHONY:
			if (!_graph->apply_internal_poly(context,
			                                 *_engine.buffer_factory(),
			                                 *_engine.maid(),
			                                 value.get<int32_t>())) {
				_status = Status::INTERNAL_ERROR;
			}
			break;
		case SpecialType::CONTROL_BINDING:
			if (port) {
				if (!_engine.control_bindings()->set_port_binding(context, port, _binding, value)) {
					_status = Status::BAD_VALUE;
				}
			} else if (block) {
				if (uris.ingen_Internal == block->plugin_impl()->type()) {
					block->learn();
				}
			}
			break;
        case SpecialType::PRESET:
	        block->set_enabled(false);
			break;
		case SpecialType::NONE:
			if (port) {
				if (!strcmp(uris.lv2_minimum.c_str(), key.c_str())) {
					port->set_minimum(value);
				} else if (!strcmp(uris.lv2_maximum.c_str(), key.c_str())) {
					port->set_maximum(value);
				}
			}
		case SpecialType::LOADED_BUNDLE:
			break;
		}
	}
}

void
Delta::post_process()
{
	if (_state) {
		BlockImpl* block = dynamic_cast<BlockImpl*>(_object);
		if (block) {
			block->apply_state(_engine.sync_worker(), _state);
			block->set_enabled(true);
		}
		lilv_state_free(_state);
	}

	Broadcaster::Transfer t(*_engine.broadcaster());

	if (_create_event) {
		_create_event->post_process();
		if (_create_event->status() != Status::SUCCESS) {
			return;  // Creation failed, nothing else to do
		}
	}

	for (auto& s : _set_events) {
		if (s->synthetic() || s->status() != Status::SUCCESS) {
			s->post_process();  // Set failed, report error
		}
	}

	if (respond() == Status::SUCCESS) {
		_update.send(_engine.broadcaster());

		switch (_type) {
		case Type::SET:
			/* Kludge to avoid feedback for set events only.  The GUI
			   depends on put responses to e.g. initially place blocks.
			   Some more sensible way of controlling this is needed. */
			if (_mode == Mode::NORMAL) {
				_engine.broadcaster()->set_ignore_client(_request_client);
			}
			_engine.broadcaster()->set_property(
				_subject,
				_properties.begin()->first,
				_properties.begin()->second);
			if (_mode == Mode::NORMAL) {
				_engine.broadcaster()->clear_ignore_client();
			}
			break;
		case Type::PUT:
			if (_type == Type::PUT && _subject.substr(0, 5) == "file:") {
				// Preset save
				ClientUpdate response;
				response.put(_preset->uri(), _preset->properties());
				response.send(_engine.broadcaster());
			} else {
				// Graph object put
				_engine.broadcaster()->put(_subject, _properties, _context);
			}
			break;
		case Type::PATCH:
			_engine.broadcaster()->delta(_subject, _remove, _properties, _context);
			break;
		}
	}
}

void
Delta::undo(Interface& target)
{
	if (_create_event) {
		_create_event->undo(target);
	} else if (_type == Type::PATCH) {
		target.delta(_subject, _added, _removed, _context);
	} else if (_type == Type::SET || _type == Type::PUT) {
		if (_removed.size() == 1) {
			target.set_property(_subject,
			                    _removed.begin()->first,
			                    _removed.begin()->second,
			                    _context);
		} else if (_removed.empty()) {
			target.delta(_subject, _added, {}, _context);
		} else {
			target.put(_subject, _removed, _context);
		}
	}
}

Event::Execution
Delta::get_execution() const
{
	return _block ? Execution::ATOMIC : Execution::NORMAL;
}

} // namespace Events
} // namespace Server
} // namespace Ingen
