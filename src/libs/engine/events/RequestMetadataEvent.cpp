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

#include "RequestMetadataEvent.h"
#include <string>
#include "Responder.h"
#include "Engine.h"
#include "GraphObject.h"
#include "ObjectStore.h"
#include "interface/ClientInterface.h"
#include "ClientBroadcaster.h"
using std::string;

namespace Ingen {


RequestMetadataEvent::RequestMetadataEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& node_path, const string& key)
: QueuedEvent(engine, responder, timestamp),
  m_path(node_path),
  m_key(key),
  m_object(NULL),
  m_client(SharedPtr<ClientInterface>())
{
}


void
RequestMetadataEvent::pre_process()
{
	m_client = _engine.broadcaster()->client(_responder->client_key());
	
	if (m_client) {
		m_object = _engine.object_store()->find(m_path);
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
		if (!m_object) {
			string msg = "Unable to find metadata subject ";
			msg += m_path;
			_responder->respond_error(msg);
		} else {
			_responder->respond_ok();
			m_client->metadata_update(m_path, m_key, m_value);
		}
	} else {
		_responder->respond_error("Unknown client");
	}
}


} // namespace Ingen

