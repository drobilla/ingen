/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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

#include "SetMetadataEvent.h"
#include <string>
#include "Responder.h"
#include "Engine.h"
#include "ClientBroadcaster.h"
#include "GraphObject.h"
#include "ObjectStore.h"

using std::string;

namespace Ingen {


SetMetadataEvent::SetMetadataEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& path, const string& key, const Atom& value)
: QueuedEvent(engine, responder, timestamp),
  _path(path),
  _key(key),
  _value(value),
  _object(NULL)
{
}


void
SetMetadataEvent::pre_process()
{
	_object = _engine.object_store()->find(_path);
	if (_object == NULL) {
		QueuedEvent::pre_process();
		return;
	}

	_object->set_metadata(_key, _value);

	QueuedEvent::pre_process();
}


void
SetMetadataEvent::execute(SampleCount nframes, FrameTime start, FrameTime end)
{
	QueuedEvent::execute(nframes, start, end);
	// Do nothing
}


void
SetMetadataEvent::post_process()
{
	if (_object == NULL) {
		string msg = "Unable to find object ";
		msg += _path;
		_responder->respond_error(msg);
	} else {
		_responder->respond_ok();
		_engine.broadcaster()->send_metadata_update(_path, _key, _value);
	}
}


} // namespace Ingen
