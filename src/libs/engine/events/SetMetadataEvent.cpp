/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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
#include "EngineStore.hpp"

using std::string;

namespace Ingen {


SetMetadataEvent::SetMetadataEvent(
		Engine&              engine,
		SharedPtr<Responder> responder,
		SampleCount          timestamp,
		bool                 property,
		const string&        path,
		const string&        key,
		const Atom&          value)
	: QueuedEvent(engine, responder, timestamp)
	, _error(NO_ERROR)
	, _special_type(NONE)
	, _property(property)
	, _path(path)
	, _key(key)
	, _value(value)
	, _object(NULL)
{
}


void
SetMetadataEvent::pre_process()
{
	if (!Path::is_valid(_path)) {
		_error = INVALID_PATH;
		QueuedEvent::pre_process();
		return;
	}
	
	_object = _engine.engine_store()->find_object(_path);
	if (_object == NULL) {
		QueuedEvent::pre_process();
		return;
	}

	if (_property)
		_object->set_property(_key, _value);
	else
		_object->set_variable(_key, _value);
	
	if (_key == "ingen:broadcast") {
		std::cout << "BROADCAST" << std::endl;
		_special_type = ENABLE_BROADCAST;
	}

	QueuedEvent::pre_process();
}


void
SetMetadataEvent::execute(ProcessContext& context)
{
	if (_special_type == ENABLE_BROADCAST) {
		PortImpl* port = dynamic_cast<PortImpl*>(_object);
		if (port)
			port->broadcast(_value.get_bool());
	}

	QueuedEvent::execute(context);
	// Do nothing
}


void
SetMetadataEvent::post_process()
{
	if (_error == INVALID_PATH) {
		_responder->respond_error((boost::format("Invalid path %1% setting %2%") % _path % _key).str());
	} else if (_object == NULL) {
		string msg = (boost::format("Unable to find object %1% to set %2%") % _path % _key).str();
		_responder->respond_error(msg);
	} else {
		_responder->respond_ok();
		if (_property)
			_engine.broadcaster()->send_property_change(_path, _key, _value);
		else
			_engine.broadcaster()->send_variable_change(_path, _key, _value);
	}
}


} // namespace Ingen
