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

#include "RequestPortValueEvent.h"
#include <string>
#include "Responder.h"
#include "Om.h"
#include "OmApp.h"
#include "interface/ClientInterface.h"
#include "PortBase.h"
#include "PortInfo.h"
#include "ObjectStore.h"
#include "ClientBroadcaster.h"

using std::string;

namespace Om {


RequestPortValueEvent::RequestPortValueEvent(CountedPtr<Responder> responder, const string& port_path)
: QueuedEvent(responder),
  m_port_path(port_path),
  m_port(NULL),
  m_value(0.0),
  m_client(CountedPtr<ClientInterface>(NULL))
{
}


void
RequestPortValueEvent::pre_process()
{
	m_client = m_responder->find_client();
	m_port = om->object_store()->find_port(m_port_path);

	QueuedEvent::pre_process();
}


void
RequestPortValueEvent::execute(samplecount offset)
{
	if (m_port != NULL && m_port->port_info()->is_audio() || m_port->port_info()->is_control())
		m_value = ((PortBase<sample>*)m_port)->buffer(0)->value_at(offset);
	else 
		m_port = NULL; // triggers error response

	QueuedEvent::execute(offset);
}


void
RequestPortValueEvent::post_process()
{
	string msg;
	if (m_port) {
		m_responder->respond_error("Unable to find port for get_value responder.");
	} else if (m_client) {
		m_responder->respond_ok();
		m_client->control_change(m_port_path, m_value);
	} else {
		m_responder->respond_error("Invalid URL");
	}
}


} // namespace Om

