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

#include "interface/ClientInterface.hpp"
#include "RequestMetadataEvent.hpp"
#include "Responder.hpp"
#include "Engine.hpp"
#include "GraphObjectImpl.hpp"
#include "EngineStore.hpp"
#include "ClientBroadcaster.hpp"
#include "PortImpl.hpp"
#include "PluginImpl.hpp"
#include "AudioBuffer.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {

using namespace Shared;


RequestMetadataEvent::RequestMetadataEvent(Engine&              engine,
	                                       SharedPtr<Responder> responder,
	                                       SampleCount          timestamp,
	                                       bool                 is_property,
	                                       const URI&           subject,
	                                       const URI&           key)
	: QueuedEvent(engine, responder, timestamp)
	, _error(NO_ERROR)
	, _special_type(NONE)
	, _uri(subject)
	, _key(key)
	, _resource(0)
	, _is_property(is_property)
{
}


void
RequestMetadataEvent::pre_process()
{
	const bool is_object = (_uri.scheme() == Path::scheme && Path::is_valid(_uri.str()));
	if (_responder->client()) {
		if (is_object)
			_resource = _engine.engine_store()->find_object(Path(_uri.str()));
		else
			_resource = _engine.node_factory()->plugin(_uri);

		if (!_resource) {
			QueuedEvent::pre_process();
			return;
		}
	}

	if (_key.str() == "ingen:value")
		_special_type = PORT_VALUE;
	else if (!is_object || _is_property)
		_value = _resource->get_property(_key);
	else
		_value = dynamic_cast<GraphObjectImpl*>(_resource)->get_variable(_key);

	QueuedEvent::pre_process();
}


void
RequestMetadataEvent::execute(ProcessContext& context)
{
	QueuedEvent::execute(context);
	if (_special_type == PORT_VALUE) {
		PortImpl* port = dynamic_cast<PortImpl*>(_resource);
		if (port) {
			if (port->type() == DataType::CONTROL || port->type() == DataType::AUDIO)
				_value = ((AudioBuffer*)port->buffer(0))->value_at(0); // TODO: offset
		} else {
			_resource = 0;
		}
	}
}


void
RequestMetadataEvent::post_process()
{
	if (_responder->client()) {
		if (_special_type == PORT_VALUE) {
			if (_resource) {
				_responder->respond_ok();
				_responder->client()->set_port_value(_uri.str(), _value);
			} else {
				const string msg = "Get value for non-port " + _uri.str();
				_responder->respond_error(msg);
			}
		} else if (!_resource) {
			const string msg = "Unable to find subject " + _uri.str();
			_responder->respond_error(msg);
		} else {
			_responder->respond_ok();
			_responder->client()->set_variable(_uri, _key, _value);
		}
	} else {
		_responder->respond_error("Unknown client");
	}
}


} // namespace Ingen

