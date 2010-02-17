/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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

#include <string>
#include <boost/format.hpp>
#include "raul/log.hpp"
#include "interface/PortType.hpp"
#include "shared/LV2URIMap.hpp"
#include "ClientBroadcaster.hpp"
#include "ControlBindings.hpp"
#include "CreateNode.hpp"
#include "CreatePatch.hpp"
#include "CreatePort.hpp"
#include "Engine.hpp"
#include "EngineStore.hpp"
#include "GraphObjectImpl.hpp"
#include "PatchImpl.hpp"
#include "PluginImpl.hpp"
#include "PortImpl.hpp"
#include "Request.hpp"
#include "SetMetadata.hpp"
#include "SetPortValue.hpp"

#define LOG(s) s << "[SetMetadata] "

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Events {

using namespace Shared;
typedef Shared::Resource::Properties Properties;


SetMetadata::SetMetadata(
		Engine&             engine,
		SharedPtr<Request>  request,
		SampleCount         timestamp,
		bool                create,
		bool                meta,
		const URI&          subject,
		const Properties&   properties,
		const Properties&   remove)
	: QueuedEvent(engine, request, timestamp, false)
	, _error(NO_ERROR)
	, _create_event(NULL)
	, _subject(subject)
	, _properties(properties)
	, _remove(remove)
	, _object(NULL)
	, _patch(NULL)
	, _compiled_patch(NULL)
	, _create(create)
	, _is_meta(meta)
{
#if 0
	LOG(debug) << "Set " << subject << " {" << endl;
	typedef Resource::Properties::const_iterator iterator;
	for (iterator i = properties.begin(); i != properties.end(); ++i)
		LOG(debug) << "    " << i->first << " = " << i->second << " :: " << i->second.type() << endl;
	LOG(debug) << "}" << endl;

	LOG(debug) << "Unset " << subject << " {" << endl;
	typedef Resource::Properties::const_iterator iterator;
	for (iterator i = remove.begin(); i != remove.end(); ++i)
		LOG(debug) << "    " << i->first << " = " << i->second << " :: " << i->second.type() << endl;
	LOG(debug) << "}" << endl;
#endif
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

	bool is_graph_object = (_subject.scheme() == Path::scheme && Path::is_valid(_subject.str()));

	_object = is_graph_object
			? _engine.engine_store()->find_object(Path(_subject.str()))
			: _object = _engine.node_factory()->plugin(_subject);

	if (!_object && (!is_graph_object || !_create)) {
		_error = NOT_FOUND;
		QueuedEvent::pre_process();
		return;
	}

	const LV2URIMap& uris = *_engine.world()->uris.get();

	if (is_graph_object && !_object) {
		Path path(_subject.str());
		bool is_patch = false, is_node = false, is_port = false, is_output = false;
		PortType data_type(PortType::UNKNOWN);
		ResourceImpl::type(_properties, is_patch, is_node, is_port, is_output, data_type);

		// Create a separate request without a source so EventSource isn't unblocked twice
		SharedPtr<Request> sub_request(new Request(NULL, _request->client(), _request->id()));

		if (is_patch) {
			uint32_t poly = 1;
			iterator p = _properties.find(uris.ingen_polyphony);
			if (p != _properties.end() && p->second.is_valid() && p->second.type() == Atom::INT)
				poly = p->second.get_int32();
			_create_event = new CreatePatch(_engine, sub_request, _time,
					path, poly, _properties);
		} else if (is_node) {
			const iterator p = _properties.find(uris.rdf_instanceOf);
			_create_event = new CreateNode(_engine, sub_request, _time,
					path, p->second.get_uri(), true, _properties);
		} else if (is_port) {
			_blocking = bool(_request);
			_create_event = new CreatePort(_engine, sub_request, _time,
					path, data_type.uri(), is_output, _properties);
		}
		if (_create_event)
			_create_event->pre_process();
		else
			_error = BAD_OBJECT_TYPE;
		QueuedEvent::pre_process();
		return;
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

	for (Properties::iterator p = _properties.begin(); p != _properties.end(); ++p) {
		const Raul::URI&  key   = p->first;
		const Raul::Atom& value = p->second;
		SpecialType       op    = NONE;
		if (obj) {
			Resource& resource = _is_meta ? obj->meta() : *obj;
			resource.add_property(key, value);

			PortImpl* port = dynamic_cast<PortImpl*>(_object);
			if (port) {
				if (key == uris.ingen_broadcast) {
					if (value.type() == Atom::BOOL) {
						op = ENABLE_BROADCAST;
					} else {
						_error = BAD_VALUE_TYPE;
					}
				} else if (key == uris.ingen_value) {
					PortImpl* port = dynamic_cast<PortImpl*>(_object);
					if (port) {
						SetPortValue* ev = new SetPortValue(_engine, _request, _time, port, value);
						ev->pre_process();
						_set_events.push_back(ev);
					} else {
						_error = BAD_OBJECT_TYPE;
					}
				} else if (key == uris.ingen_controlBinding) {
					PortImpl* port = dynamic_cast<PortImpl*>(_object);
					if (port && port->type() == Shared::PortType::CONTROL) {
						if (value == uris.wildcard) {
							_engine.control_bindings()->learn(port);
						} else if (value.type() == Atom::DICT) {
							op = CONTROL_BINDING;
						} else {
							_error = BAD_VALUE_TYPE;
						}
					} else {
						_error = BAD_OBJECT_TYPE;
					}
				}
			} else if ((_patch = dynamic_cast<PatchImpl*>(_object))) {
				if (key == uris.ingen_enabled) {
					if (value.type() == Atom::BOOL) {
						op = ENABLE;
						if (value.get_bool() && !_patch->compiled_patch())
							_compiled_patch = _patch->compile();
					} else {
						_error = BAD_VALUE_TYPE;
					}
				} else if (key == uris.ingen_polyphonic) {
					if (value.type() == Atom::BOOL) {
						op = POLYPHONIC;
					} else {
						_error = BAD_VALUE_TYPE;
					}
				} else if (key == uris.ingen_polyphony) {
					if (value.type() == Atom::INT) {
						op = POLYPHONY;
						_blocking = true;
						_patch->prepare_internal_poly(*_engine.buffer_factory(), value.get_int32());
					} else {
						_error = BAD_VALUE_TYPE;
					}
				}
			}
		}

		if (_error != NO_ERROR) {
			_error_predicate += key.str();
			break;
		}

		_types.push_back(op);
	}

	for (Properties::iterator p = _remove.begin(); p != _remove.end(); ++p) {
		const Raul::URI&  key   = p->first;
		const Raul::Atom& value = p->second;
		if (key == uris.ingen_controlBinding && value == uris.wildcard) {
			PortImpl* port = dynamic_cast<PortImpl*>(_object);
			if (port)
				_old_bindings = _engine.control_bindings()->remove(port);
		}
		_object->remove_property(key, value);
	}

	QueuedEvent::pre_process();
}


void
SetMetadata::execute(ProcessContext& context)
{
	if (_error != NO_ERROR) {
		QueuedEvent::execute(context);
		return;
	}

	if (_create_event) {
		QueuedEvent::execute(context);
		_create_event->execute(context);
		if (_blocking)
			_request->unblock();
		return;
	}

	for (SetEvents::iterator i = _set_events.begin(); i != _set_events.end(); ++i)
		(*i)->execute(context);

	GraphObjectImpl* const object = dynamic_cast<GraphObjectImpl*>(_object);
	NodeBase* const        node   = dynamic_cast<NodeBase*>(_object);
	PortImpl* const        port   = dynamic_cast<PortImpl*>(_object);

	std::vector<SpecialType>::const_iterator t = _types.begin();
	for (Properties::const_iterator p = _properties.begin(); p != _properties.end(); ++p, ++t) {
		const Raul::Atom& value = p->second;
		switch (*t) {
		case ENABLE_BROADCAST:
			if (port)
				port->broadcast(value.get_bool());
			break;
		case ENABLE:
			if (value.get_bool()) {
				if (!_patch->compiled_patch())
					_patch->compiled_patch(_compiled_patch);
				_patch->enable();
			} else {
				_patch->disable();
			}
			break;
		case POLYPHONIC:
			if (object)
				if (!object->set_polyphonic(*_engine.maid(), value.get_bool()))
					_error = INTERNAL;
			break;
		case POLYPHONY:
			if (!_patch->apply_internal_poly(*_engine.maid(), value.get_int32()))
				_error = INTERNAL;
			break;
		case CONTROL_BINDING:
			if (port) {
				_engine.control_bindings()->port_binding_changed(context, port);
			} else if (node) {
				if (node->plugin_impl()->type() == Shared::Plugin::Internal) {
					node->learn();
				}
			}
			break;
		case NONE:
			break;
		}
	}

	for (Properties::const_iterator p = _remove.begin(); p != _remove.end(); ++p, ++t)
		_object->remove_property(p->first, p->second);

	QueuedEvent::execute(context);

	if (_blocking)
		_request->unblock();
}


void
SetMetadata::post_process()
{
	for (SetEvents::iterator i = _set_events.begin(); i != _set_events.end(); ++i)
		(*i)->post_process();

	switch (_error) {
	case NO_ERROR:
		_request->respond_ok();
		if (_create)
			_engine.broadcaster()->put(_subject, _properties);
		else
			_engine.broadcaster()->delta(_subject, _remove, _properties);
		if (_create_event)
			_create_event->post_process();
		break;
	case NOT_FOUND:
		_request->respond_error((boost::format(
				"Unable to find object `%1%'") % _subject).str());
	case INTERNAL:
		_request->respond_error("Internal error");
		break;
	case BAD_OBJECT_TYPE:
		_request->respond_error((boost::format(
				"Bad type for object `%1%'") % _subject).str());
		break;
	case BAD_VALUE_TYPE:
		_request->respond_error((boost::format(
				"Bad metadata value type for subject `%1%' predicate `%2%'")
					% _subject % _error_predicate).str());
		break;
	}
}


} // namespace Ingen
} // namespace Events

