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

#include "DisconnectNodeEvent.h"
#include <iostream>
#include "Responder.h"
#include "Engine.h"
#include "Maid.h"
#include "List.h"
#include "Node.h"
#include "TypedConnection.h"
#include "DisconnectionEvent.h"
#include "Port.h"
#include "Array.h"
#include "InputPort.h"
#include "OutputPort.h"
#include "Patch.h"
#include "ClientBroadcaster.h"
#include "util.h"
#include "ObjectStore.h"
#include "util/Path.h"

using std::cerr; using std::endl;

namespace Ingen {


DisconnectNodeEvent::DisconnectNodeEvent(CountedPtr<Responder> responder, SampleCount timestamp, const string& node_path)
: QueuedEvent(responder, timestamp),
  m_node_path(node_path),
  m_patch(NULL),
  m_node(NULL),
  m_succeeded(true),
  m_lookup(true)
{
}


/** Internal version, disconnects parent port as well (in the case of InputNode, etc).
 */
DisconnectNodeEvent::DisconnectNodeEvent(Node* node)
: QueuedEvent(),
  m_node_path(""),
  m_patch(node->parent_patch()),
  m_node(node),
  m_succeeded(true),
  m_lookup(false)
{
}


DisconnectNodeEvent::~DisconnectNodeEvent()
{
	for (List<DisconnectionEvent*>::iterator i = m_disconnection_events.begin(); i != m_disconnection_events.end(); ++i)
		delete (*i);
}


void
DisconnectNodeEvent::pre_process()
{
	typedef List<Connection*>::const_iterator ConnectionListIterator;
	
	// cerr << "Preparing disconnection event...\n";
	
	if (m_lookup) {
		m_patch = Engine::instance().object_store()->find_patch(m_node_path.parent());
	
		if (m_patch == NULL) {
			m_succeeded = false;
			QueuedEvent::pre_process();
			return;
		}
		
		m_node = Engine::instance().object_store()->find_node(m_node_path);
		
		if (m_node == NULL) {
			m_succeeded = false;
			QueuedEvent::pre_process();
			return;
		}
	}

	Connection* c = NULL;
	for (ConnectionListIterator i = m_patch->connections().begin(); i != m_patch->connections().end(); ++i) {
		c = (*i);
		if ((c->src_port()->parent_node() == m_node || c->dst_port()->parent_node() == m_node) && !c->pending_disconnection()) {
			DisconnectionEvent* ev = new DisconnectionEvent(CountedPtr<Responder>(new Responder()), _time_stamp,
				c->src_port(), c->dst_port());
			ev->pre_process();
			m_disconnection_events.push_back(new ListNode<DisconnectionEvent*>(ev));
			c->pending_disconnection(true);
		}
	}
	
	m_succeeded = true;
	QueuedEvent::pre_process();	
}


void
DisconnectNodeEvent::execute(SampleCount offset)
{
	if (m_succeeded) {
		for (List<DisconnectionEvent*>::iterator i = m_disconnection_events.begin(); i != m_disconnection_events.end(); ++i)
			(*i)->execute(offset);
	}
	
	QueuedEvent::execute(offset);
}


void
DisconnectNodeEvent::post_process()
{
	if (m_succeeded) {
		if (_responder)
			_responder->respond_ok();
		for (List<DisconnectionEvent*>::iterator i = m_disconnection_events.begin(); i != m_disconnection_events.end(); ++i)
			(*i)->post_process();
	} else {
		if (_responder)
			_responder->respond_error("Unable to disconnect all ports.");
	}
}


} // namespace Ingen

