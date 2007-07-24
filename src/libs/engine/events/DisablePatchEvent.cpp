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

#include "DisablePatchEvent.hpp"
#include "interface/Responder.hpp"
#include "Engine.hpp"
#include "Patch.hpp"
#include "ClientBroadcaster.hpp"
#include "util.hpp"
#include "ObjectStore.hpp"
#include "Port.hpp"

namespace Ingen {


DisablePatchEvent::DisablePatchEvent(Engine& engine, SharedPtr<Shared::Responder> responder, SampleCount timestamp, const string& patch_path)
: QueuedEvent(engine, responder, timestamp),
  _patch_path(patch_path),
  _patch(NULL)
{
}


void
DisablePatchEvent::pre_process()
{
	_patch = _engine.object_store()->find_patch(_patch_path);
	
	QueuedEvent::pre_process();
}


void
DisablePatchEvent::execute(SampleCount nframes, FrameTime start, FrameTime end)
{
	QueuedEvent::execute(nframes, start, end);
	
	if (_patch != NULL)
		_patch->disable();
}


void
DisablePatchEvent::post_process()
{	
	if (_patch != NULL) {
		_responder->respond_ok();
		_engine.broadcaster()->send_patch_disable(_patch_path);
	} else {
		_responder->respond_error(string("Patch ") + _patch_path + " not found");
	}
}


} // namespace Ingen

