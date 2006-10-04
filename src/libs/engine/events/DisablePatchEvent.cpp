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

#include "DisablePatchEvent.h"
#include "Responder.h"
#include "Engine.h"
#include "Patch.h"
#include "ClientBroadcaster.h"
#include "util.h"
#include "ObjectStore.h"
#include "Port.h"

namespace Ingen {


DisablePatchEvent::DisablePatchEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& patch_path)
: QueuedEvent(engine, responder, timestamp),
  m_patch_path(patch_path),
  m_patch(NULL)
{
}


void
DisablePatchEvent::pre_process()
{
	m_patch = _engine.object_store()->find_patch(m_patch_path);
	
	QueuedEvent::pre_process();
}


void
DisablePatchEvent::execute(SampleCount nframes, FrameTime start, FrameTime end)
{
	QueuedEvent::execute(nframes, start, end);
	
	if (m_patch != NULL)
		m_patch->disable();
}


void
DisablePatchEvent::post_process()
{	
	if (m_patch != NULL) {
		_responder->respond_ok();
		_engine.broadcaster()->send_patch_disable(m_patch_path);
	} else {
		_responder->respond_error(string("Patch ") + m_patch_path + " not found");
	}
}


} // namespace Ingen

