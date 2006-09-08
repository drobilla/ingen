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

#include "RequestPluginsEvent.h"
#include "Responder.h"
#include "Engine.h"
#include "ClientBroadcaster.h"
#include "NodeFactory.h"

namespace Ingen {


RequestPluginsEvent::RequestPluginsEvent(Engine& engine, CountedPtr<Responder> responder, SampleCount timestamp)
: QueuedEvent(engine, responder, timestamp),
  m_client(CountedPtr<ClientInterface>(NULL))
{
}


void
RequestPluginsEvent::pre_process()
{
	m_client = _engine.client_broadcaster()->client(_responder->client_key());
	
	// Take a copy to send in the post processing thread (to avoid problems
	// because std::list isn't thread safe)
	m_plugins = _engine.node_factory()->plugins();

	QueuedEvent::pre_process();
}


void
RequestPluginsEvent::post_process()
{
	if (m_client) {
		_engine.client_broadcaster()->send_plugins_to(m_client.get(), m_plugins);
		_responder->respond_ok();
	} else {
		_responder->respond_error("Unable to find client to send plugins");
	}
}


} // namespace Ingen

