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

#include <vector>
#include <thread>

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

// #define DUMP 1
// #include "ingen/URIMap.hpp"

namespace Ingen {
namespace Server {
namespace Events {

typedef Resource::Properties Properties;

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
	, _compiled_graph(NULL)
	, _context(context)
	, _type(type)
	, _poly_lock(engine.store()->mutex(), std::defer_lock)
{
	if (context != Resource::Graph::DEFAULT) {
		for (auto& p : _properties) {
			p.second.set_context(context);
		}
	}

#ifdef DUMP
	std::cerr << "Delta " << subject << " : " << (int)context << " {" << std::endl;
	typedef Resource::Properties::const_iterator iterator;
	for (iterator i = properties.begin(); i != properties.end(); ++i) {
		std::cerr << "    + " << i->first
		          << " = " << engine.world()->forge().str(i->second)
		          << " :: " << engine.world()->uri_map().unmap_uri(i->second.type())
		          << std::endl;
	}
	typedef Resource::Properties::const_iterator iterator;
	for (iterator i = remove.begin(); i != remove.end(); ++i) {
		std::cerr << "    - " << i->first
		          << " = " << engine.world()->forge().str(i->second)
		          << " :: " << engine.world()->uri_map().unmap_uri(i->second.type())
		          << std::endl;
	}
	std::cerr << "}" << std::endl;
#endif
}

Delta::~Delta()
{
	for (auto& s : _set_events)
		delete s;

	delete _create_event;
}

bool
Delta::pre_process()
{
	const bool is_graph_object = Node::uri_is_path(_subject);
	const bool is_client       = (_subject == "ingen:/clients/this");
	bool       poly_changed    = false;

	// Take a writer lock while we modify the store
	std::unique_lock<std::mutex> lock(_engine.store()->mutex());

	_object = is_graph_object
		? static_cast<Ingen::Resource*>(_engine.store()->get(Node::uri_to_path(_subject)))
		: static_cast<Ingen::Resource*>(_engine.block_factory()->plugin(_subject));

	if (!_object && !is_client && (!is_graph_object || _type != Type::PUT)) {
		return Event::pre_process_done(Status::NOT_FOUND, _subject);
	}

	const Ingen::URIs& uris = _engine.world()->uris();

	if (is_graph_object && !_object) {
		Raul::Path path(Node::uri_to_path(_subject));
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
				path, is_output, _properties);
		}
		if (_create_event) {
			if (_create_event->pre_process()) {
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
			if (port)
				_old_bindings = _engine.control_bindings()->remove(port);
		}
		if (_object) {
			_object->remove_property(key, value);
		}
	}

	// Remove all added properties if this is a put
	if (_type == Type::PUT && _object) {
		for (const auto& p : _properties) {
			_object->remove_property(p.first, p.second);
		}
	}

	for (const auto& p : _properties) {
		const Raul::URI&          key   = p.first;
		const Resource::Property& value = p.second;
		SpecialType               op    = SpecialType::NONE;
		if (obj) {
			Resource& resource = *obj;
			if (value != uris.patch_wildcard) {
				resource.add_property(key, value, value.context());
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
						_engine, _request_client, _request_id, _time, port, value);
					ev->pre_process();
					_set_events.push_back(ev);
				} else if (key == uris.midi_binding) {
					if (port->is_a(PortType::CONTROL) || port->is_a(PortType::CV)) {
						if (value == uris.patch_wildcard) {
							_engine.control_bindings()->learn(port);
						} else if (value.type() == uris.atom_Object) {
							op = SpecialType::CONTROL_BINDING;
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
				}
			}

			if ((_graph = dynamic_cast<GraphImpl*>(_object))) {
				if (key == uris.ingen_enabled) {
					if (value.type() == uris.forge.Bool) {
						op = SpecialType::ENABLE;
						// FIXME: defer this until all other metadata has been processed
						if (value.get<int32_t>() && !_graph->enabled())
							_compiled_graph = _graph->compile();
					} else {
						_status = Status::BAD_VALUE_TYPE;
					}
				} else if (key == uris.ingen_polyphony) {
					if (value.type() == uris.forge.Int) {
						if (value.get<int32_t>() < 1 || value.get<int32_t>() > 128) {
							_status = Status::INVALID_POLY;
						} else {
							poly_changed = true;
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
				if (parent) {
					if (value.type() == uris.forge.Bool) {
						poly_changed = true;
						op = SpecialType::POLYPHONIC;
						obj->set_property(key, value, value.context());
						BlockImpl* block = dynamic_cast<BlockImpl*>(obj);
						if (block)
							block->set_polyphonic(value.get<int32_t>());
						if (value.get<int32_t>()) {
							obj->prepare_poly(*_engine.buffer_factory(), parent->internal_poly());
						} else {
							obj->prepare_poly(*_engine.buffer_factory(), 1);
						}
					} else {
						_status = Status::BAD_VALUE_TYPE;
					}
				} else {
					_status = Status::BAD_OBJECT_TYPE;
				}
			}
		} else if (is_client && key == uris.ingen_broadcast) {
			_engine.broadcaster()->set_broadcast(
				_request_client->uri(), value.get<int32_t>());
		}

		if (_status != Status::NOT_PREPARED) {
			break;
		}

		_types.push_back(op);
	}

	if (poly_changed) {
		lock.unlock();
		_poly_lock.lock();
	}

	return Event::pre_process_done(
		_status == Status::NOT_PREPARED ? Status::SUCCESS : _status,
		_subject);
}

void
Delta::execute(ProcessContext& context)
{
	if (_status != Status::SUCCESS) {
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
						_graph->set_compiled_graph(_compiled_graph);
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
				object->apply_poly(
					context, *_engine.maid(), parent->internal_poly_process());
			} else {
				object->apply_poly(context, *_engine.maid(), 1);
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
				_engine.control_bindings()->port_binding_changed(context, port, value);
			} else if (block) {
				if (block->plugin_impl()->type() == Plugin::Internal) {
					block->learn();
				}
			}
			break;
		case SpecialType::NONE:
			if (port) {
				if (key == uris.lv2_minimum) {
					port->set_minimum(value);
				} else if (key == uris.lv2_maximum) {
					port->set_maximum(value);
				}
			}
			break;
		}
	}
}

void
Delta::post_process()
{
	if (_poly_lock.owns_lock()) {
		_poly_lock.unlock();
	}

	Broadcaster::Transfer t(*_engine.broadcaster());

	if (_create_event) {
		_create_event->post_process();
		if (_create_event->status() != Status::SUCCESS) {
			return;  // Creation failed, nothing else to do
		}
	}

	for (auto& s : _set_events) {
		if (s->status() != Status::SUCCESS) {
			s->post_process();  // Set failed, report error
		}
	}

	if (respond() == Status::SUCCESS) {
		switch (_type) {
		case Type::SET:
			/* Kludge to avoid feedback for set events only.  The GUI
			   depends on put responses to e.g. initially place blocks.
			   Some more sensible way of controlling this is needed. */
			_engine.broadcaster()->set_ignore_client(_request_client);
			_engine.broadcaster()->set_property(
				_subject,
				(*_properties.begin()).first,
				(*_properties.begin()).second);
			_engine.broadcaster()->clear_ignore_client();
			break;
		case Type::PUT:
			_engine.broadcaster()->put(_subject, _properties, _context);
			break;
		case Type::PATCH:
			_engine.broadcaster()->delta(_subject, _remove, _properties);
			break;
		}
	}
}

} // namespace Events
} // namespace Server
} // namespace Ingen
