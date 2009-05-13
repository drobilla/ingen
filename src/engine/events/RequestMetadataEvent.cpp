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

#include "interface/ClientInterface.hpp"
#include "RequestMetadataEvent.hpp"
#include "Responder.hpp"
#include "Engine.hpp"
#include "GraphObjectImpl.hpp"
#include "EngineStore.hpp"
#include "ClientBroadcaster.hpp"
#include "PortImpl.hpp"
#include "AudioBuffer.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {

using namespace Shared;


RequestMetadataEvent::RequestMetadataEvent(Engine&              engine,
	                                       SharedPtr<Responder> responder,
	                                       SampleCount          timestamp,
	                                       bool                 property,
	                                       const Path&          node_path,
	                                       const URI&           key)
	: QueuedEvent(engine, responder, timestamp)
	, _special_type(NONE)
	, _path(node_path)
	, _property(property)
	, _key(key)
	, _object(NULL)
{
}


void
RequestMetadataEvent::pre_process()
{
	if (_responder->client()) {
		_object = _engine.engine_store()->find_object(_path);
		if (_object == NULL) {
			QueuedEvent::pre_process();
			return;
		}
	}

	if (_key.str() == "ingen:value")
		_special_type = PORT_VALUE;
	else if (_property)
		_value = _object->get_property(_key);
	else
		_value = _object->get_variable(_key);
	
	QueuedEvent::pre_process();
}


void
RequestMetadataEvent::execute(ProcessContext& context)
{
	QueuedEvent::execute(context);
	if (_special_type == PORT_VALUE) {
		PortImpl* port = dynamic_cast<PortImpl*>(_object);
		if (port) {
			if (port->type() == DataType::CONTROL || port->type() == DataType::AUDIO)
				_value = ((AudioBuffer*)port->buffer(0))->value_at(0); // TODO: offset
		} else {
			_object = 0;
		}
	}
}


void
RequestMetadataEvent::post_process()
{
	if (_responder->client()) {
		if (_special_type == PORT_VALUE) {
			if (_object) {
				_responder->respond_ok();
				_responder->client()->set_port_value(_path, _value);
			} else {
				const string msg = "Get value for non-port " + _path.str();
				_responder->respond_error(msg);
			}
		} else if (!_object) {
			const string msg = "Unable to find variable subject " + _path.str();
			_responder->respond_error(msg);
		} else {
			_responder->respond_ok();
			_responder->client()->set_variable(_path, _key, _value);
		}
	} else {
		_responder->respond_error("Unknown client");
	}
}


} // namespace Ingen

