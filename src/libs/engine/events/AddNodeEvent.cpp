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

#include "AddNodeEvent.h"
#include "Responder.h"
#include "Patch.h"
#include "Node.h"
#include "Tree.h"
#include "Plugin.h"
#include "Om.h"
#include "OmApp.h"
#include "Patch.h"
#include "NodeFactory.h"
#include "ClientBroadcaster.h"
#include "Maid.h"
#include "util/Path.h"
#include "ObjectStore.h"
#include "util/Path.h"
#include "Port.h"

namespace Om {


AddNodeEvent::AddNodeEvent(CountedPtr<Responder> responder, const string& path, Plugin* plugin, bool poly)
: QueuedEvent(responder),
  m_path(path),
  m_plugin(plugin),
  m_poly(poly),
  m_patch(NULL),
  m_node(NULL),
  m_process_order(NULL),
  m_node_already_exists(false)
{
}


AddNodeEvent::~AddNodeEvent()
{
	delete m_plugin;
}


void
AddNodeEvent::pre_process()
{
	if (om->object_store()->find(m_path) != NULL) {
		m_node_already_exists = true;
		QueuedEvent::pre_process();
		return;
	}

	m_patch = om->object_store()->find_patch(m_path.parent());

	if (m_patch != NULL) {
		if (m_poly)
			m_node = om->node_factory()->load_plugin(m_plugin, m_path.name(), m_patch->internal_poly(), m_patch);
		else
			m_node = om->node_factory()->load_plugin(m_plugin, m_path.name(), 1, m_patch);
		
		if (m_node != NULL) {
			m_node->activate();
		
			// This can be done here because the audio thread doesn't touch the
			// node tree - just the process order array
			m_patch->add_node(new ListNode<Node*>(m_node));
			m_node->add_to_store();
				
			if (m_patch->process())
				m_process_order = m_patch->build_process_order();
		}
	}
	QueuedEvent::pre_process();
}


void
AddNodeEvent::execute(samplecount offset)
{
	QueuedEvent::execute(offset);

	if (m_node != NULL) {
		m_node->add_to_patch();
		
		if (m_patch->process_order() != NULL)
			om->maid()->push(m_patch->process_order());
		m_patch->process_order(m_process_order);
	}
}


void
AddNodeEvent::post_process()
{
	string msg;
	if (m_node_already_exists) {
		msg = string("Could not create node - ").append(m_path);// + " already exists.";
		m_responder->respond_error(msg);
	} else if (m_patch == NULL) {
		msg = "Could not find patch '" + m_path.parent() +"' for add_node.";
		m_responder->respond_error(msg);
	} else if (m_node == NULL) {
		msg = "Unable to load node ";
		msg.append(m_path).append(" (you're missing the plugin \"").append(
			m_plugin->lib_name()).append(":").append(m_plugin->plug_label()).append("\")");;
		m_responder->respond_error(msg);
	} else {
		m_responder->respond_ok();
		//om->client_broadcaster()->send_node_creation_messages(m_node);
		om->client_broadcaster()->send_node(m_node);
	}
}


} // namespace Om

