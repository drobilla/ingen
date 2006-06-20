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

#include "RequestMetadataEvent.h"
#include <string>
#include "Responder.h"
#include "Om.h"
#include "OmApp.h"
#include "GraphObject.h"
#include "ObjectStore.h"
#include "interface/ClientInterface.h"
#include "ClientBroadcaster.h"
using std::string;

namespace Om {


RequestMetadataEvent::RequestMetadataEvent(CountedPtr<Responder> responder, const string& node_path, const string& key)
: QueuedEvent(responder),
  m_path(node_path),
  m_key(key),
  m_value(""),
  m_object(NULL),
  m_client(CountedPtr<ClientInterface>(NULL))
{
}


void
RequestMetadataEvent::pre_process()
{
	m_client = m_responder->find_client();
	
	if (m_client) {
		m_object = om->object_store()->find(m_path);
		if (m_object == NULL) {
			QueuedEvent::pre_process();
			return;
		}
	}

	m_value = m_object->get_metadata(m_key);
	
	QueuedEvent::pre_process();
}


void
RequestMetadataEvent::post_process()
{
	if (m_client) {
		if (m_value == "") {
			string msg = "Unable to find object ";
			msg += m_path;
			m_responder->respond_error(msg);
		} else {
			m_responder->respond_ok();
			m_client->metadata_update(m_path, m_key, m_value);
		}
	} else {
		m_responder->respond_error("Unknown client");
	}
}


} // namespace Om

