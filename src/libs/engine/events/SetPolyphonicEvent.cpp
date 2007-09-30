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

#include <raul/Maid.hpp>
#include "SetPolyphonicEvent.hpp"
#include "Responder.hpp"
#include "Engine.hpp"
#include "Patch.hpp"
#include "ClientBroadcaster.hpp"
#include "util.hpp"
#include "ObjectStore.hpp"
#include "Port.hpp"
#include "Node.hpp"
#include "Connection.hpp"
#include "QueuedEventSource.hpp"

namespace Ingen {


SetPolyphonicEvent::SetPolyphonicEvent(Engine& engine, SharedPtr<Responder> responder, FrameTime time, QueuedEventSource* source, const string& path, bool poly)
: QueuedEvent(engine, responder, time, true, source),
  _path(path),
  _object(NULL),
  _poly(poly)
{
}


void
SetPolyphonicEvent::pre_process()
{
	_object = _engine.object_store()->find_object(_path);
	
	 QueuedEvent::pre_process();
}


void
SetPolyphonicEvent::execute(ProcessContext& context)
{
	QueuedEvent::execute(context);
	 
	if (_object)
		 _object->set_polyphonic(*_engine.maid(), _poly);
	
	_source->unblock();
}


void
SetPolyphonicEvent::post_process()
{
	if (_object) {
		_responder->respond_ok();
		_engine.broadcaster()->send_polyphonic(_path, _poly);
	} else {
		_responder->respond_error("Unable to find object");
	}
}


} // namespace Ingen

