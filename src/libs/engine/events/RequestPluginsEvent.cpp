/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "RequestPluginsEvent.h"
#include "Responder.h"
#include "Om.h"
#include "OmApp.h"
#include "ClientBroadcaster.h"

namespace Om {


RequestPluginsEvent::RequestPluginsEvent(CountedPtr<Responder> responder)
: QueuedEvent(responder),
  m_client(CountedPtr<ClientInterface>(NULL))
{
}


void
RequestPluginsEvent::pre_process()
{
	m_client = m_responder->find_client();
	
	QueuedEvent::pre_process();
}


void
RequestPluginsEvent::post_process()
{
	if (m_client) {
		om->client_broadcaster()->send_plugins_to(m_client.get());
		m_responder->respond_ok();
	} else {
		m_responder->respond_error("Invalid URL");
	}
}


} // namespace Om

