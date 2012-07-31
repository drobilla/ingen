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

#include "ingen/URIs.hpp"
#include "raul/Maid.hpp"

#include "Broadcaster.hpp"
#include "ControlBindings.hpp"
#include "CreateNode.hpp"
#include "CreatePatch.hpp"
#include "CreatePort.hpp"
#include "Delta.hpp"
#include "Engine.hpp"
#include "EngineStore.hpp"
#include "PatchImpl.hpp"
#include "PluginImpl.hpp"
#include "PortImpl.hpp"
#include "PortType.hpp"
#include "SetPortValue.hpp"

#define LOG(s) s << "[Delta] "

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
	, _patch(NULL)
	, _compiled_patch(NULL)
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
	LOG(Raul::info) << "Delta " << subject << " : " << context << " {" << std::endl;
	typedef Resource::Properties::const_iterator iterator;
	for (iterator i = properties.begin(); i != properties.end(); ++i) {
		LOG(Raul::info) << "    + " << i->first
		                << " = " << engine.world()->forge().str(i->second)
		                << " :: " << engine.world()->uri_map().unmap_uri(i->second.type()) << std::endl;
	}
	typedef Resource::Properties::const_iterator iterator;
	for (iterator i = remove.begin(); i != remove.end(); ++i) {
		LOG(Raul::info) << "    - " << i->first
		                << " = " << engine.world()->forge().str(i->second)
		                << " :: " << engine.world()->uri_map().unmap_uri(i->second.type()) << std::endl;
	}
	LOG(Raul::info) << "}" << std::endl;
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

	const bool is_graph_object = Raul::Path::is_path(_subject);

	// Take a writer lock while we modify the store
	Glib::RWLock::WriterLock lock(_engine.engine_store()->lock());

	_object = is_graph_object
		? _engine.engine_store()->find_object(Raul::Path(_subject.str()))
		: static_cast<Ingen::Resource*>(_engine.node_factory()->plugin(_subject));

	if (!_object && (!is_graph_object || !_create)) {
		return Event::pre_process_done(NOT_FOUND, _subject);
	}

	const Ingen::URIs& uris = _engine.world()->uris();

	if (is_graph_object && !_object) {
		Raul::Path path(_subject.str());
		bool is_patch = false, is_node = false, is_port = false, is_output = false;
		Ingen::Resource::type(uris, _properties, is_patch, is_node, is_port, is_output);

		if (is_patch) {
			_create_event = new CreatePatch(
				_engine, _request_client, _request_id, _time, path, _properties);
		} else if (is_node) {
			_create_event = new CreateNode(
				_engine, _request_client, _request_id, _time, path, _properties);
		} else if (is_port) {
			_create_event = new CreatePort(
				_engine, _request_client, _request_id, _time,
				path, is_output, _properties);
		}
		if (_create_event) {
			_create_event->pre_process();
			// Grab the object for applying properties, if the create-event succeeded
			_object = _engine.engine_store()->find_object(Raul::Path(_subject.str()));
		} else {
			return Event::pre_process_done(BAD_OBJECT_TYPE, _subject);
		}
	}

	_types.reserve(_properties.size());

	GraphObjectImpl* obj = dynamic_cast<GraphObjectImpl*>(_object);

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
			resource.add_property(key, value, value.context());

			PortImpl* port = dynamic_cast<PortImpl*>(_object);
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
			} else if ((_patch = dynamic_cast<PatchImpl*>(_object))) {
				if (key == uris.ingen_enabled) {
					if (value.type() == uris.forge.Bool) {
						op = ENABLE;
						// FIXME: defer this until all other metadata has been processed
						if (value.get_bool() && !_patch->enabled())
							_compiled_patch = _patch->compile();
					} else {
						_status = BAD_VALUE_TYPE;
					}
				} else if (key == uris.ingen_polyphony) {
					if (value.type() == uris.forge.Int) {
						op = POLYPHONY;
						_patch->prepare_internal_poly(*_engine.buffer_factory(), value.get_int32());
					} else {
						_status = BAD_VALUE_TYPE;
					}
				}
			} else if (key == uris.ingen_polyphonic) {
				PatchImpl* parent = dynamic_cast<PatchImpl*>(obj->parent());
				if (parent) {
					if (value.type() == uris.forge.Bool) {
						op = POLYPHONIC;
						obj->set_property(key, value, value.context());
						NodeImpl* node = dynamic_cast<NodeImpl*>(obj);
						if (node)
							node->set_polyphonic(value.get_bool());
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

	GraphObjectImpl* const object = dynamic_cast<GraphObjectImpl*>(_object);
	NodeImpl* const        node   = dynamic_cast<NodeImpl*>(_object);
	PortImpl* const        port   = dynamic_cast<PortImpl*>(_object);

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
				if (_compiled_patch) {
					_engine.maid()->push(_patch->compiled_patch());
					_patch->compiled_patch(_compiled_patch);
				}
				_patch->enable();
			} else {
				_patch->disable(context);
			}
			break;
		case POLYPHONIC: {
			PatchImpl* parent = reinterpret_cast<PatchImpl*>(object->parent());
			if (value.get_bool()) {
				object->apply_poly(
					context, *_engine.maid(), parent->internal_poly_process());
			} else {
				object->apply_poly(context, *_engine.maid(), 1);
			}
		} break;
		case POLYPHONY:
			if (!_patch->apply_internal_poly(context,
			                                 *_engine.buffer_factory(),
			                                 *_engine.maid(),
			                                 value.get_int32())) {
				_status = INTERNAL_ERROR;
			}
			break;
		case CONTROL_BINDING:
			if (port) {
				_engine.control_bindings()->port_binding_changed(context, port, value);
			} else if (node) {
				if (node->plugin_impl()->type() == Plugin::Internal) {
					node->learn();
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

