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

#include "raul/Maid.hpp"
#include "SetPolyphonicEvent.hpp"
#include "Responder.hpp"
#include "Engine.hpp"
#include "PatchImpl.hpp"
#include "ClientBroadcaster.hpp"
#include "util.hpp"
#include "EngineStore.hpp"
#include "PortImpl.hpp"
#include "NodeImpl.hpp"
#include "ConnectionImpl.hpp"
#include "QueuedEventSource.hpp"

namespace Ingen {


SetPolyphonicEvent::SetPolyphonicEvent(Engine& engine, SharedPtr<Responder> responder, FrameTime time, QueuedEventSource* source, const string& path, bool poly)
: QueuedEvent(engine, responder, time, true, source),
  _path(path),
  _object(NULL),
  _poly(poly),
  _success(false)
{
}


void
SetPolyphonicEvent::pre_process()
{
	_object = _engine.engine_store()->find_object(_path);
	
	 QueuedEvent::pre_process();
}


void
SetPolyphonicEvent::execute(ProcessContext& context)
{
	QueuedEvent::execute(context);
	 
	if (_object)
		 _success = _object->set_polyphonic(*_engine.maid(), _poly);
	
	_source->unblock();
}


void
SetPolyphonicEvent::post_process()
{
	if (_object) {
		if (_success) {
			_responder->respond_ok();
			_engine.broadcaster()->send_property_change(_path, "ingen:polyphonic", _poly);
		} else {
			_responder->respond_error("Unable to set object as polyphonic");
		}
	} else {
		_responder->respond_error("Unable to find object to set polyphonic");
	}
}


} // namespace Ingen

