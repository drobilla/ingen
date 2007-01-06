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

#include "DisconnectPortEvent.h"
#include <iostream>
#include "Responder.h"
#include "Engine.h"
#include "Maid.h"
#include "List.h"
#include "Node.h"
#include "Connection.h"
#include "DisconnectionEvent.h"
#include "Port.h"
#include "Array.h"
#include "InputPort.h"
#include "OutputPort.h"
#include "Patch.h"
#include "ClientBroadcaster.h"
#include "util.h"
#include "ObjectStore.h"
#include "raul/Path.h"

using std::cerr; using std::endl;

namespace Ingen {


DisconnectPortEvent::DisconnectPortEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& port_path)
: QueuedEvent(engine, responder, timestamp),
  m_port_path(port_path),
  m_patch(NULL),
  m_port(NULL),
  m_process_order(NULL),
  m_succeeded(true),
  m_lookup(true)
{
}


DisconnectPortEvent::DisconnectPortEvent(Engine& engine, Port* port)
: QueuedEvent(engine),
  m_port_path(port->path()),
  m_patch((port->parent_node() == NULL) ? NULL : port->parent_node()->parent_patch()),
  m_port(port),
  m_process_order(NULL),
  m_succeeded(true),
  m_lookup(false)
{
	//cerr << "DisconnectPortEvent(Engine& engine, )\n";
}


DisconnectPortEvent::~DisconnectPortEvent()
{
	for (List<DisconnectionEvent*>::iterator i = m_disconnection_events.begin(); i != m_disconnection_events.end(); ++i)
		delete (*i);
}


void
DisconnectPortEvent::pre_process()
{
	// cerr << "Preparing disconnection event...\n";
	
	if (m_lookup) {
		m_patch = _engine.object_store()->find_patch(m_port_path.parent().parent());
	
		if (m_patch == NULL) {
			m_succeeded = false;
			QueuedEvent::pre_process();
			return;
		}
		
		m_port = _engine.object_store()->find_port(m_port_path);
		
		if (m_port == NULL) {
			m_succeeded = false;
			QueuedEvent::pre_process();
			return;
		}
	}

	if (m_patch == NULL) {
		m_succeeded = false;
		QueuedEvent::pre_process();
		return;
	}
	
	Connection* c = NULL;
	for (List<Connection*>::const_iterator i = m_patch->connections().begin(); i != m_patch->connections().end(); ++i) {
		c = (*i);
		if ((c->src_port() == m_port || c->dst_port() == m_port) && !c->pending_disconnection()) {
			DisconnectionEvent* ev = new DisconnectionEvent(_engine, SharedPtr<Responder>(new Responder()), _time,
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
DisconnectPortEvent::execute(SampleCount nframes, FrameTime start, FrameTime end)
{
	QueuedEvent::execute(nframes, start, end);

	if (m_succeeded) {
		for (List<DisconnectionEvent*>::iterator i = m_disconnection_events.begin(); i != m_disconnection_events.end(); ++i)
			(*i)->execute(nframes, start, end);
	}
}


void
DisconnectPortEvent::post_process()
{
	if (m_succeeded) {
		if (_responder)
			_responder->respond_ok();
		for (List<DisconnectionEvent*>::iterator i = m_disconnection_events.begin(); i != m_disconnection_events.end(); ++i)
			(*i)->post_process();
	} else {
		if (_responder)
			_responder->respond_error("Unable to disconnect port.");
	}
}


} // namespace Ingen

