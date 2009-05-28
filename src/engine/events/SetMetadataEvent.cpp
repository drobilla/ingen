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

#include "SetMetadataEvent.hpp"
#include <string>
#include <boost/format.hpp>
#include "Responder.hpp"
#include "Engine.hpp"
#include "PortImpl.hpp"
#include "ClientBroadcaster.hpp"
#include "GraphObjectImpl.hpp"
#include "PatchImpl.hpp"
#include "PluginImpl.hpp"
#include "EngineStore.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {


SetMetadataEvent::SetMetadataEvent(
		Engine&              engine,
		SharedPtr<Responder> responder,
		SampleCount          timestamp,
		bool                 meta,
		const URI&           subject,
		const URI&           key,
		const Atom&          value)
	: QueuedEvent(engine, responder, timestamp)
	, _error(NO_ERROR)
	, _subject(subject)
	, _object(NULL)
	, _patch(NULL)
	, _compiled_patch(NULL)
	, _is_meta(meta)
	, _success(false)
{
	cerr << "SET " << subject << " : " << key << " = " << value << endl;
	assert(value.type() != Atom::URI || strcmp(value.get_uri(), "lv2:ControlPort"));
	_properties.insert(make_pair(key, value));
}


SetMetadataEvent::SetMetadataEvent(
		Engine&                             engine,
		SharedPtr<Responder>                responder,
		SampleCount                         timestamp,
		bool                                meta,
		const URI&                          subject,
		const Shared::Resource::Properties& properties)
	: QueuedEvent(engine, responder, timestamp)
	, _error(NO_ERROR)
	, _subject(subject)
	, _properties(properties)
	, _object(NULL)
	, _patch(NULL)
	, _compiled_patch(NULL)
	, _is_meta(meta)
	, _success(false)
{
}


void
SetMetadataEvent::pre_process()
{
	if (_subject.scheme() == Path::scheme && Path::is_valid(_subject.str()))
		_object = _engine.engine_store()->find_object(Path(_subject.str()));
	else
		_object = _engine.node_factory()->plugin(_subject);

	if (_object == NULL) {
		_error = NOT_FOUND;
		QueuedEvent::pre_process();
		return;
	}

	/*cerr << "SET " << _object->path() << (_property ? " PROP " : " VAR ")
	  <<	_key << " :: " << _value.type() << endl;*/

	_types.reserve(_properties.size());
	typedef Shared::Resource::Properties Properties;
	for (Properties::iterator p = _properties.begin(); p != _properties.end(); ++p) {
		const Raul::URI&  key   = p->first;
		const Raul::Atom& value = p->second;
		GraphObjectImpl* obj = dynamic_cast<GraphObjectImpl*>(_object);
		if (obj) {
			if (_is_meta)
				obj->meta().set_property(key, value);
			else
				obj->set_property(key, value);

			_patch = dynamic_cast<PatchImpl*>(_object);

			if (key.str() == "ingen:broadcast") {
				_types.push_back(ENABLE_BROADCAST);
			} else if (_patch) {
				if (key.str() == "ingen:enabled") {
					if (value.type() == Atom::BOOL) {
						_types.push_back(ENABLE);
						if (value.get_bool() && !_patch->compiled_patch())
							_compiled_patch = _patch->compile();
					} else {
						_error = BAD_TYPE;
					}
				} else if (key.str() == "ingen:polyphonic") {
					if (value.type() == Atom::BOOL) {
						_types.push_back(POLYPHONIC);
					} else {
						_error = BAD_TYPE;
					}
				} else if (key.str() == "ingen:polyphony") {
					if (value.type() == Atom::INT) {
						_types.push_back(POLYPHONY);
						_patch->prepare_internal_poly(value.get_int32());
					} else {
						_error = BAD_TYPE;
					}
				} else {
					_types.push_back(NONE);
				}
				if (_error != NO_ERROR)
					break;
			}
		} else {
			_types.push_back(NONE);
		}

		_object->set_property(key, value);
	}

	QueuedEvent::pre_process();
}


void
SetMetadataEvent::execute(ProcessContext& context)
{
	if (_error != NO_ERROR)
		return;

	typedef Shared::Resource::Properties Properties;
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
SetMetadataEvent::post_process()
{
	switch (_error) {
	case NO_ERROR:
		_responder->respond_ok();
		_engine.broadcaster()->send_put(_subject, _properties);
		break;
	case NOT_FOUND:
		_responder->respond_error((boost::format(
				"Unable to find object '%1%'") % _subject).str());
	case INTERNAL:
		_responder->respond_error("Internal error");
		break;
	case BAD_TYPE:
		_responder->respond_error("Bad type for predicate");
		break;
	}
}


} // namespace Ingen

