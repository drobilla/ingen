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

#include "RequestAllObjectsEvent.h"
#include "Responder.h"
#include "Engine.h"
#include "ObjectSender.h"
#include "ClientBroadcaster.h"
#include "ObjectStore.h"

namespace Ingen {


RequestAllObjectsEvent::RequestAllObjectsEvent(Engine& engine, CountedPtr<Responder> responder, SampleCount timestamp)
: QueuedEvent(engine, responder, timestamp)
{
}


void
RequestAllObjectsEvent::pre_process()
{
	m_client = _engine.broadcaster()->client(_responder->client_key());
	
	QueuedEvent::pre_process();
}


void
RequestAllObjectsEvent::post_process()
{
	if (m_client) {
		_responder->respond_ok();

		// Everything is a child of the root patch, so this sends it all
		Patch* root = _engine.object_store()->find_patch("/");
		if (root)
			ObjectSender::send_patch(m_client.get(), root);

	} else {
		_responder->respond_error("Unable to find client to send all objects");
	}
}


} // namespace Ingen

