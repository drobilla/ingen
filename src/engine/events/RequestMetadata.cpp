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

#include "raul/IntrusivePtr.hpp"
#include "interface/ClientInterface.hpp"
#include "events/RequestMetadata.hpp"
#include "shared/LV2Object.hpp"
#include "shared/LV2URIMap.hpp"
#include "AudioBuffer.hpp"
#include "ClientBroadcaster.hpp"
#include "Engine.hpp"
#include "EngineStore.hpp"
#include "GraphObjectImpl.hpp"
#include "ObjectBuffer.hpp"
#include "PluginImpl.hpp"
#include "PortImpl.hpp"
#include "ProcessContext.hpp"
#include "Request.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Events {

using namespace Shared;


RequestMetadata::RequestMetadata(Engine&            engine,
                                 SharedPtr<Request> request,
                                 SampleCount        timestamp,
                                 bool               is_meta,
                                 const URI&         subject,
                                 const URI&         key)
	: QueuedEvent(engine, request, timestamp)
	, _special_type(NONE)
	, _uri(subject)
	, _key(key)
	, _resource(0)
	, _is_meta(is_meta)
{
}


void
RequestMetadata::pre_process()
{
	const bool is_object = Path::is_path(_uri);
	if (_request->client()) {
		if (is_object)
			_resource = _engine.engine_store()->find_object(Path(_uri.str()));
		else
			_resource = _engine.node_factory()->plugin(_uri);

		if (!_resource) {
			QueuedEvent::pre_process();
			return;
		}
	}

	GraphObjectImpl* obj = dynamic_cast<GraphObjectImpl*>(_resource);
	if (obj) {
		if (_key == _engine.world()->uris()->ingen_value)
			_special_type = PORT_VALUE;
		else if (_is_meta)
			_value = obj->meta().get_property(_key);
		else
			_value = obj->get_property(_key);
	} else {
		_value = _resource->get_property(_key);
	}

	QueuedEvent::pre_process();
}


void
RequestMetadata::execute(ProcessContext& context)
{
	QueuedEvent::execute(context);
	if (_special_type == PORT_VALUE) {
		PortImpl* port = dynamic_cast<PortImpl*>(_resource);
		if (port) {
			IntrusivePtr<AudioBuffer> abuf = PtrCast<AudioBuffer>(port->buffer(0));
			if (abuf) {
				_value = abuf->value_at(0);
			} else {
				IntrusivePtr<ObjectBuffer> obuf = PtrCast<ObjectBuffer>(port->buffer(0));
				if (obuf) {
					LV2Object::to_atom(*_engine.world()->uris().get(), obuf->object(), _value);
				}
			}
		} else {
			_resource = 0;
		}
	}
}


void
RequestMetadata::post_process()
{
	if (_request->client()) {
		if (_special_type == PORT_VALUE) {
			if (_resource) {
				_request->respond_ok();
				_request->client()->set_property(_uri.str(),
						_engine.world()->uris()->ingen_value, _value);
			} else {
				const string msg = "Get value for non-port " + _uri.str();
				_request->respond_error(msg);
			}
		} else if (!_resource) {
			const string msg = "Unable to find subject " + _uri.str();
			_request->respond_error(msg);
		} else {
			_request->respond_ok();
			_request->client()->set_property(_uri, _key, _value);
		}
	} else {
		_request->respond_error("Unknown client");
	}
}


} // namespace Ingen
} // namespace Events

