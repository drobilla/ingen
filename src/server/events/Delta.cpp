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

#include <glibmm/thread.h>

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

Delta::Delta(Engine&              engine,
             SharedPtr<Interface> client,
             int32_t              id,
             SampleCount          timestamp,
             bool                 create,
             Resource::Graph      context,
             const Raul::URI&     subject,
             const Properties&    properties,
             const Properties&    remove)
	: Event(engine, client, id, timestamp)
	, _create_event(NULL)
	, _subject(subject)
	, _properties(properties)
	, _remove(remove)
	, _object(NULL)
	, _graph(NULL)
	, _compiled_graph(NULL)
	, _context(context)
	, _create(create)
{
	if (context != Resource::DEFAULT) {
		for (Properties::iterator i = _properties.begin();
		     i != _properties.end();
		     ++i) {
			i->second.set_context(context);
		}
	}

#ifdef DUMP
	std::cerr << "Delta " << subject << " : " << context << " {" << std::endl;
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
	for (SetEvents::iterator i = _set_events.begin(); i != _set_events.end(); ++i)
		delete *i;

	delete _create_event;
}

bool
Delta::pre_process()
{
	typedef Properties::const_iterator iterator;

	const bool is_graph_object = Node::uri_is_path(_subject);

	// Take a writer lock while we modify the store
	Glib::RWLock::WriterLock lock(_engine.store()->lock());

	_object = is_graph_object
		? static_cast<Ingen::Resource*>(_engine.store()->get(Node::uri_to_path(_subject)))
		: static_cast<Ingen::Resource*>(_engine.block_factory()->plugin(_subject));

	if (!_object && (!is_graph_object || !_create)) {
		return Event::pre_process_done(NOT_FOUND, _subject);
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
			_create_event->pre_process();
			// Grab the object for applying properties, if the create-event succeeded
			_object = _engine.store()->get(path);
		} else {
			return Event::pre_process_done(BAD_OBJECT_TYPE, _subject);
		}
	}

	_types.reserve(_properties.size());

	NodeImpl* obj = dynamic_cast<NodeImpl*>(_object);

	for (Properties::const_iterator p = _remove.begin(); p != _remove.end(); ++p) {
		const Raul::URI&  key   = p->first;
		const Raul::Atom& value = p->second;
		if (key == uris.ingen_controlBinding && value == uris.wildcard) {
			PortImpl* port = dynamic_cast<PortImpl*>(_object);
			if (port)
				_old_bindings = _engine.control_bindings()->remove(port);
		}
		_object->remove_property(key, value);
	}

	for (Properties::const_iterator p = _properties.begin(); p != _properties.end(); ++p) {
		const Raul::URI&          key   = p->first;
		const Resource::Property& value = p->second;
		SpecialType               op    = NONE;
		if (obj) {
			Resource& resource = *obj;
			if (value != uris.wildcard) {
				resource.add_property(key, value, value.context());
			}

			BlockImpl* block = NULL;
			PortImpl*  port  = dynamic_cast<PortImpl*>(_object);
			if (port) {
				if (key == uris.ingen_broadcast) {
					if (value.type() == uris.forge.Bool) {
						op = ENABLE_BROADCAST;
					} else {
						_status = BAD_VALUE_TYPE;
					}
				} else if (key == uris.ingen_value) {
					SetPortValue* ev = new SetPortValue(
						_engine, _request_client, _request_id, _time, port, value);
					ev->pre_process();
					_set_events.push_back(ev);
				} else if (key == uris.ingen_controlBinding) {
					if (port->is_a(PortType::CONTROL) || port->is_a(PortType::CV)) {
						if (value == uris.wildcard) {
							_engine.control_bindings()->learn(port);
						} else if (value.type() == uris.atom_Blank) {
							op = CONTROL_BINDING;
						} else {
							_status = BAD_VALUE_TYPE;
						}
					} else {
						_status = BAD_OBJECT_TYPE;
					}
				}
			} else if ((block = dynamic_cast<BlockImpl*>(_object))) {
				if (key == uris.ingen_controlBinding && value == uris.wildcard) {
					op = CONTROL_BINDING;  // Internal block learn
				}
			} else if ((_graph = dynamic_cast<GraphImpl*>(_object))) {
				if (key == uris.ingen_enabled) {
					if (value.type() == uris.forge.Bool) {
						op = ENABLE;
						// FIXME: defer this until all other metadata has been processed
						if (value.get_bool() && !_graph->enabled())
							_compiled_graph = _graph->compile();
					} else {
						_status = BAD_VALUE_TYPE;
					}
				} else if (key == uris.ingen_polyphony) {
					if (value.type() == uris.forge.Int) {
						if (value.get_int32() < 1 || value.get_int32() > 128) {
							_status = INVALID_POLY;
						} else {
							op = POLYPHONY;
							_graph->prepare_internal_poly(
								*_engine.buffer_factory(), value.get_int32());
						}
					} else {
						_status = BAD_VALUE_TYPE;
					}
				}
			} else if (key == uris.ingen_polyphonic) {
				GraphImpl* parent = dynamic_cast<GraphImpl*>(obj->parent());
				if (parent) {
					if (value.type() == uris.forge.Bool) {
						op = POLYPHONIC;
						obj->set_property(key, value, value.context());
						BlockImpl* block = dynamic_cast<BlockImpl*>(obj);
						if (block)
							block->set_polyphonic(value.get_bool());
						if (value.get_bool()) {
							obj->prepare_poly(*_engine.buffer_factory(), parent->internal_poly());
						} else {
							obj->prepare_poly(*_engine.buffer_factory(), 1);
						}
					} else {
						_status = BAD_VALUE_TYPE;
					}
				} else {
					_status = BAD_OBJECT_TYPE;
				}
			}
		}

		if (_status != NOT_PREPARED) {
			break;
		}

		_types.push_back(op);
	}

	return Event::pre_process_done(_status == NOT_PREPARED ? SUCCESS : _status,
	                               _subject);
}

void
Delta::execute(ProcessContext& context)
{
	if (_status) {
		return;
	}

	const Ingen::URIs& uris = _engine.world()->uris();

	if (_create_event) {
		_create_event->set_time(_time);
		_create_event->execute(context);
	}

	for (SetEvents::iterator i = _set_events.begin(); i != _set_events.end(); ++i) {
		(*i)->set_time(_time);
		(*i)->execute(context);
	}

	NodeImpl* const  object = dynamic_cast<NodeImpl*>(_object);
	BlockImpl* const block  = dynamic_cast<BlockImpl*>(_object);
	PortImpl* const  port   = dynamic_cast<PortImpl*>(_object);

	std::vector<SpecialType>::const_iterator t = _types.begin();
	for (Properties::const_iterator p = _properties.begin(); p != _properties.end(); ++p, ++t) {
		const Raul::URI&  key   = p->first;
		const Raul::Atom& value = p->second;
		switch (*t) {
		case ENABLE_BROADCAST:
			if (port) {
				port->broadcast(value.get_bool());
			}
			break;
		case ENABLE:
			if (value.get_bool()) {
				if (_compiled_graph) {
					_engine.maid()->dispose(_graph->compiled_graph());
					_graph->compiled_graph(_compiled_graph);
				}
				_graph->enable();
			} else {
				_graph->disable(context);
			}
			break;
		case POLYPHONIC: {
			GraphImpl* parent = reinterpret_cast<GraphImpl*>(object->parent());
			if (value.get_bool()) {
				object->apply_poly(
					context, *_engine.maid(), parent->internal_poly_process());
			} else {
				object->apply_poly(context, *_engine.maid(), 1);
			}
		} break;
		case POLYPHONY:
			if (!_graph->apply_internal_poly(context,
			                                 *_engine.buffer_factory(),
			                                 *_engine.maid(),
			                                 value.get_int32())) {
				_status = INTERNAL_ERROR;
			}
			break;
		case CONTROL_BINDING:
			if (port) {
				_engine.control_bindings()->port_binding_changed(context, port, value);
			} else if (block) {
				if (block->plugin_impl()->type() == Plugin::Internal) {
					block->learn();
				}
			}
			break;
		case NONE:
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
	for (SetEvents::iterator i = _set_events.begin(); i != _set_events.end(); ++i)
		(*i)->post_process();

	if (!_status) {
		if (_create_event) {
			_create_event->post_process();
		} else {
			respond();
			if (_create) {
				_engine.broadcaster()->put(_subject, _properties, _context);
			} else {
				_engine.broadcaster()->delta(_subject, _remove, _properties);
			}
		}
	} else {
		respond();
	}
}

} // namespace Events
} // namespace Server
} // namespace Ingen

