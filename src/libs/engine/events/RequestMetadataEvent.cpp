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

#include "RequestMetadataEvent.hpp"
#include <string>
#include "Responder.hpp"
#include "Engine.hpp"
#include "GraphObjectImpl.hpp"
#include "ObjectStore.hpp"
#include "interface/ClientInterface.hpp"
#include "ClientBroadcaster.hpp"
using std::string;

namespace Ingen {


RequestMetadataEvent::RequestMetadataEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& node_path, const string& key)
: QueuedEvent(engine, responder, timestamp),
  _path(node_path),
  _key(key),
  _object(NULL)
{
}


void
RequestMetadataEvent::pre_process()
{
	if (_responder->client()) {
		_object = _engine.object_store()->find_object(_path);
		if (_object == NULL) {
			QueuedEvent::pre_process();
			return;
		}
	}

	_value = _object->get_variable(_key);
	
	QueuedEvent::pre_process();
}


void
RequestMetadataEvent::post_process()
{
	if (_responder->client()) {
		if (!_object) {
			string msg = "Unable to find variable subject ";
			msg += _path;
			_responder->respond_error(msg);
		} else {
			_responder->respond_ok();
			_responder->client()->variable_change(_path, _key, _value);
		}
	} else {
		_responder->respond_error("Unknown client");
	}
}


} // namespace Ingen

