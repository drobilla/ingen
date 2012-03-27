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


#include <string>

#include <boost/format.hpp>

#include "raul/log.hpp"
#include "raul/Maid.hpp"

#include "ingen/shared/LV2URIMap.hpp"
#include "ingen/shared/URIs.hpp"

#include "ClientBroadcaster.hpp"
#include "ControlBindings.hpp"
#include "CreateNode.hpp"
#include "CreatePatch.hpp"
#include "CreatePort.hpp"
#include "Driver.hpp"
#include "Engine.hpp"
#include "EngineStore.hpp"
#include "GraphObjectImpl.hpp"
#include "PatchImpl.hpp"
#include "PluginImpl.hpp"
#include "PortImpl.hpp"
#include "PortType.hpp"
#include "SetMetadata.hpp"
#include "SetPortValue.hpp"

#define LOG(s) s << "[SetMetadata] "

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Server {
namespace Events {

typedef Resource::Properties Properties;

SetMetadata::SetMetadata(Engine&           engine,
                         Interface*        client,
                         int32_t           id,
                         SampleCount       timestamp,
                         bool              create,
                         Resource::Graph   context,
                         const URI&        subject,
                         const Properties& properties,
                         const Properties& remove)
	: Event(engine, client, id, timestamp)
	, _create_event(NULL)
	, _subject(subject)
	, _properties(properties)
	, _remove(remove)
	, _object(NULL)
	, _patch(NULL)
	, _compiled_patch(NULL)
	, _create(create)
	, _context(context)
	, _lock(engine.engine_store()->lock(), Glib::NOT_LOCK)
{
	if (context != Resource::DEFAULT) {
		Resource::set_context(_properties, context);
	}

	/*
	LOG(info) << "Set " << subject << " : " << context << " {" << endl;
	typedef Resource::Properties::const_iterator iterator;
	for (iterator i = properties.begin(); i != properties.end(); ++i)
		LOG(info) << "    " << i->first << " = " << i->second << " :: " << i->second.type() << endl;
	LOG(info) << "}" << endl;

	LOG(info) << "Unset " << subject << " {" << endl;
	typedef Resource::Properties::const_iterator iterator;
	for (iterator i = remove.begin(); i != remove.end(); ++i)
		LOG(info) << "    " << i->first << " = " << i->second << " :: " << i->second.type() << endl;
	LOG(info) << "}" << endl;
	*/
}

SetMetadata::~SetMetadata()
{
	for (SetEvents::iterator i = _set_events.begin(); i != _set_events.end(); ++i)
		delete *i;

	delete _create_event;
}

void
SetMetadata::pre_process()
{
	typedef Properties::const_iterator iterator;

	const bool is_graph_object = Path::is_path(_subject);

	_lock.acquire();

	_object = is_graph_object
		? _engine.engine_store()->find_object(Path(_subject.str()))
		: static_cast<Shared::ResourceImpl*>(_engine.node_factory()->plugin(_subject));

	if (!_object && (!is_graph_object || !_create)) {
		_status = NOT_FOUND;
		Event::pre_process();
		return;
	}

	const Ingen::Shared::URIs& uris = *_engine.world()->uris().get();

	if (is_graph_object && !_object) {
		Path path(_subject.str());
		bool is_patch = false, is_node = false, is_port = false, is_output = false;
		Shared::ResourceImpl::type(uris, _properties, is_patch, is_node, is_port, is_output);

		if (is_patch) {
			uint32_t poly = 1;
			iterator p = _properties.find(uris.ingen_polyphony);
			if (p != _properties.end() && p->second.is_valid() && p->second.type() == uris.forge.Int)
				poly = p->second.get_int32();
			_create_event = new CreatePatch(_engine, _request_client, _request_id, _time,
			                                path, poly, _properties);
		} else if (is_node) {
			const iterator p = _properties.find(uris.rdf_instanceOf);
			_create_event = new CreateNode(_engine, _request_client, _request_id, _time,
			                               path, p->second.get_uri(), _properties);
		} else if (is_port) {
			_create_event = new CreatePort(_engine, _request_client, _request_id, _time,
			                               path, is_output, _properties);
		}
		if (_create_event) {
			_create_event->pre_process();
			// Grab the object for applying properties, if the create-event succeeded
			_object = _engine.engine_store()->find_object(Path(_subject.str()));
		} else {
			_status = BAD_OBJECT_TYPE;
		}
	}

	_types.reserve(_properties.size());

	GraphObjectImpl* obj = dynamic_cast<GraphObjectImpl*>(_object);

#if 0
	// If we're replacing (i.e. this is a PUT, not a POST), first remove all properties
	// with keys we will later set.  This must be done first so a PUT with several properties
	// of the same predicate (e.g. rdf:type) retains the multiple values.  Only previously
	// existing properties should be replaced
	if (_replace)
		for (Properties::iterator p = _properties.begin(); p != _properties.end(); ++p)
			obj->properties().erase(p->first);
#endif

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

	for (Properties::iterator p = _properties.begin(); p != _properties.end(); ++p) {
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
						} else if (value.type() == uris.forge.Dict) {
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

		if (_status != SUCCESS) {
			_error_predicate += key.str();
			break;
		}

		_types.push_back(op);
	}

	if (!_create_event) {
		_lock.release();
	}

	Event::pre_process();
}

void
SetMetadata::execute(ProcessContext& context)
{
	if (_status != SUCCESS) {
		Event::execute(context);
		return;
	}

	const Ingen::Shared::URIs& uris = *_engine.world()->uris().get();

	if (_create_event) {
		_create_event->execute(context);
	}

	for (SetEvents::iterator i = _set_events.begin(); i != _set_events.end(); ++i)
		(*i)->execute(context);

	GraphObjectImpl* const object = dynamic_cast<GraphObjectImpl*>(_object);
	NodeImpl* const        node   = dynamic_cast<NodeImpl*>(_object);
	PortImpl* const        port   = dynamic_cast<PortImpl*>(_object);

	std::vector<SpecialType>::const_iterator t = _types.begin();
	for (Properties::const_iterator p = _properties.begin(); p != _properties.end(); ++p, ++t) {
		const Raul::URI&  key   = p->first;
		const Raul::Atom& value = p->second;
		switch (*t) {
		case ENABLE_BROADCAST:
			if (port)
				port->broadcast(value.get_bool());
			break;
		case ENABLE:
			if (value.get_bool()) {
				if (_compiled_patch) {
					_engine.maid()->push(_patch->compiled_patch());
					_patch->compiled_patch(_compiled_patch);
				}
				_patch->enable();
			} else {
				_patch->disable();
			}
			break;
		case POLYPHONIC:
			{
				PatchImpl* parent = reinterpret_cast<PatchImpl*>(object->parent());
				if (value.get_bool())
					object->apply_poly(*_engine.maid(), parent->internal_poly());
				else
					object->apply_poly(*_engine.maid(), 1);
			}
			break;
		case POLYPHONY:
			if (_patch->internal_poly() != static_cast<uint32_t>(value.get_int32()) &&
					!_patch->apply_internal_poly(_engine.driver()->context(),
						*_engine.buffer_factory(),
						*_engine.maid(), value.get_int32())) {
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

	Event::execute(context);
}

void
SetMetadata::post_process()
{
	for (SetEvents::iterator i = _set_events.begin(); i != _set_events.end(); ++i)
		(*i)->post_process();

	if (!_status) {
		if (_create_event) {
			_create_event->post_process();
		} else {
			respond(SUCCESS);
			if (_create) {
				_engine.broadcaster()->put(_subject, _properties, _context);
			} else {
				_engine.broadcaster()->delta(_subject, _remove, _properties);
			}
		}
	} else {
		respond(_status);
	}

	if (_create_event) {
		_lock.release();
	}
}

} // namespace Server
} // namespace Ingen
} // namespace Events

