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

#include "DSSIConfigureEvent.h"
#include "Engine.h"
#include "Node.h"
#include "ClientBroadcaster.h"
#include "Plugin.h"
#include "ObjectStore.h"

namespace Ingen {


DSSIConfigureEvent::DSSIConfigureEvent(Engine& engine, CountedPtr<Responder> responder, SampleCount timestamp, const string& node_path, const string& key, const string& val)
: QueuedEvent(engine, responder, timestamp),
  m_node_path(node_path),
  m_key(key),
  m_val(val),
  m_node(NULL)
{
}


void
DSSIConfigureEvent::pre_process()
{
	Node* node = _engine.object_store()->find_node(m_node_path);

	if (node != NULL && node->plugin()->type() == Plugin::DSSI) {
		m_node = (DSSINode*)node;
		m_node->configure(m_key, m_val);
	}

	QueuedEvent::pre_process();
}

	
void
DSSIConfigureEvent::execute(SampleCount nframes, FrameTime start, FrameTime end)
{
	// Nothing.
}


void
DSSIConfigureEvent::post_process()
{
	if (m_node == NULL) {
		cerr << "Unable to find DSSI node " << m_node_path << endl;
	} else {
		string key = "dssi-configure--";
		key += m_key;
		_engine.client_broadcaster()->send_metadata_update(m_node_path, key, m_val);
	}
}


} // namespace Ingen

