/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "RequestAllObjectsEvent.h"
#include "Responder.h"
#include "Om.h"
#include "OmApp.h"
#include "ObjectSender.h"

namespace Om {


RequestAllObjectsEvent::RequestAllObjectsEvent(CountedPtr<Responder> responder, samplecount timestamp)
: QueuedEvent(responder, timestamp),
  m_client(CountedPtr<ClientInterface>(NULL))
{
}


void
RequestAllObjectsEvent::pre_process()
{
	m_client = _responder->find_client();
	
	QueuedEvent::pre_process();
}


void
RequestAllObjectsEvent::post_process()
{
	if (m_client) {
		_responder->respond_ok();
		ObjectSender::send_all(m_client.get());
	} else {
		_responder->respond_error("Invalid URL");
	}
}


} // namespace Om

