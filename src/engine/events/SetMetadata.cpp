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
#include "interface/PortType.hpp"
#include "ClientBroadcaster.hpp"
#include "CreateNode.hpp"
#include "CreatePatch.hpp"
#include "CreatePort.hpp"
#include "Engine.hpp"
#include "EngineStore.hpp"
#include "GraphObjectImpl.hpp"
#include "PatchImpl.hpp"
#include "PluginImpl.hpp"
#include "PortImpl.hpp"
#include "QueuedEventSource.hpp"
#include "Responder.hpp"
#include "SetMetadata.hpp"
#include "SetPortValue.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Events {

using namespace Shared;
typedef Shared::Resource::Properties Properties;


SetMetadata::SetMetadata(
		Engine&               engine,
		SharedPtr<Responder>  responder,
		SampleCount           timestamp,
		QueuedEventSource*    source,
		bool                  replace,
		bool                  meta,
		const URI&            subject,
		const Properties&     properties)
	: QueuedEvent(engine, responder, timestamp, false, source)
	, _error(NO_ERROR)
	, _create_event(NULL)
	, _subject(subject)
	, _properties(properties)
	, _object(NULL)
	, _patch(NULL)
	, _compiled_patch(NULL)
	, _replace(replace)
	, _is_meta(meta)
	, _success(false)
{
}


SetMetadata::~SetMetadata()
{
	for (SetEvents::iterator i = _set_events.begin(); i != _set_events.end(); ++i)
		delete *i;
}


void
SetMetadata::pre_process()
{
	typedef Properties::const_iterator iterator;

	bool is_graph_object = (_subject.scheme() == Path::scheme && Path::is_valid(_subject.str()));

	_object = is_graph_object
			? _engine.engine_store()->find_object(Path(_subject.str()))
			: _object = _engine.node_factory()->plugin(_subject);

	if (!_object && !is_graph_object) {
		_error = NOT_FOUND;
		QueuedEvent::pre_process();
		return;
	}

	if (is_graph_object && !_object) {
		Path path(_subject.str());
		bool is_patch = false, is_node = false, is_port = false, is_output = false;
		PortType data_type(PortType::UNKNOWN);
		ResourceImpl::type(_properties, is_patch, is_node, is_port, is_output, data_type);
		if (is_patch) {
			uint32_t poly = 1;
			iterator p = _properties.find("ingen:polyphony");
			if (p != _properties.end() && p->second.is_valid() && p->second.type() == Atom::INT)
				poly = p->second.get_int32();
			_create_event = new CreatePatch(_engine, _responder, _time,
					path, poly, _properties);
		} else if (is_node) {
			const iterator p = _properties.find("rdf:instanceOf");
			_create_event = new CreateNode(_engine, _responder, _time,
					path, p->second.get_uri(), true, _properties);
		} else if (is_port) {
			_blocking = true;
			_create_event = new CreatePort(_engine, _responder, _time,
					path, data_type.uri(), is_output, _source, _properties);
		}
		if (_create_event)
			_create_event->pre_process();
		else
			_error = BAD_TYPE;
		QueuedEvent::pre_process();
		return;
	}

	_types.reserve(_properties.size());

	GraphObjectImpl* obj = dynamic_cast<GraphObjectImpl*>(_object);

	// If we're replacing (i.e. this is a PUT, not a POST), first remove all properties
	// with keys we will later set.  This must be done first so a PUT with several properties
	// of the same predicate (e.g. rdf:type) retains the multiple values.  Only previously
	// existing properties should be replaced
	if (_replace)
		for (Properties::iterator p = _properties.begin(); p != _properties.end(); ++p)
			obj->properties().erase(p->first);

	for (Properties::iterator p = _properties.begin(); p != _properties.end(); ++p) {
		const Raul::URI&  key   = p->first;
		const Raul::Atom& value = p->second;
		SpecialType       op    = NONE;
		if (obj) {
			Resource& resource = _is_meta ? obj->meta() : *obj;
			resource.add_property(key, value);

			_patch = dynamic_cast<PatchImpl*>(_object);

			if (key.str() == "ingen:broadcast") {
				op = ENABLE_BROADCAST;
			} else if (_patch) {
				if (key.str() == "ingen:enabled") {
					if (value.type() == Atom::BOOL) {
						op = ENABLE;
						if (value.get_bool() && !_patch->compiled_patch())
							_compiled_patch = _patch->compile();
					} else {
						_error = BAD_TYPE;
					}
				} else if (key.str() == "ingen:polyphonic") {
					if (value.type() == Atom::BOOL) {
						op = POLYPHONIC;
					} else {
						_error = BAD_TYPE;
					}
				} else if (key.str() == "ingen:polyphony") {
					if (value.type() == Atom::INT) {
						op = POLYPHONY;
						_patch->prepare_internal_poly(*_engine.buffer_factory(), value.get_int32());
					} else {
						_error = BAD_TYPE;
					}
				}
			} else if (key.str() == "ingen:value") {
				PortImpl* port = dynamic_cast<PortImpl*>(_object);
				if (port) {
					SetPortValue* ev = new SetPortValue(_engine, _responder, _time, port, value);
					ev->pre_process();
					_set_events.push_back(ev);
				} else {
					cerr << "WARNING: Set value for non-port " << _object->uri() << endl;
				}
			}
		}

		if (_error != NO_ERROR)
			break;

		_types.push_back(op);
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
			_source->unblock();
		return;
	}

	for (SetEvents::iterator i = _set_events.begin(); i != _set_events.end(); ++i)
		(*i)->execute(context);

	std::vector<SpecialType>::const_iterator t = _types.begin();
	for (Properties::iterator p = _properties.begin(); p != _properties.end(); ++p, ++t) {
		const Raul::Atom& value  = p->second;
		PortImpl*         port   = 0;
		GraphObjectImpl*  object = 0;
		switch (*t) {
		case ENABLE_BROADCAST:
			if ((port = dynamic_cast<PortImpl*>(_object)))
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
			if ((object = dynamic_cast<GraphObjectImpl*>(_object)))
				if (!object->set_polyphonic(*_engine.maid(), value.get_bool()))
					_error = INTERNAL;
			break;
		case POLYPHONY:
			if (!_patch->apply_internal_poly(*_engine.maid(), value.get_int32()))
				_error = INTERNAL;
			break;
		default:
			_success = true;
		}
	}

	QueuedEvent::execute(context);
}


void
SetMetadata::post_process()
{
	for (SetEvents::iterator i = _set_events.begin(); i != _set_events.end(); ++i)
		(*i)->post_process();

	switch (_error) {
	case NO_ERROR:
		_responder->respond_ok();
		_engine.broadcaster()->send_put(_subject, _properties);
		if (_create_event)
			_create_event->post_process();
		break;
	case NOT_FOUND:
		_responder->respond_error((boost::format(
				"Unable to find object '%1%'") % _subject).str());
	case INTERNAL:
		_responder->respond_error("Internal error");
		break;
	case BAD_TYPE:
		_responder->respond_error("Bad type");
		break;
	}
}


} // namespace Ingen
} // namespace Events

