/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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
#include "raul/Path.h"

using std::cerr; using std::endl;

namespace Ingen {


DisconnectNodeEvent::DisconnectNodeEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& node_path)
: QueuedEvent(engine, responder, timestamp),
  _node_path(node_path),
  _patch(NULL),
  _node(NULL),
  _succeeded(true),
  _lookup(true)
{
}


/** Internal version, disconnects parent port as well (in the case of InputNode, etc).
 */
DisconnectNodeEvent::DisconnectNodeEvent(Engine& engine, Node* node)
: QueuedEvent(engine),
  _node_path(node->path()),
  _patch(node->parent_patch()),
  _node(node),
  _succeeded(true),
  _lookup(false)
{
}


DisconnectNodeEvent::~DisconnectNodeEvent()
{
	for (List<DisconnectionEvent*>::iterator i = _disconnection_events.begin(); i != _disconnection_events.end(); ++i)
		delete (*i);
}


void
DisconnectNodeEvent::pre_process()
{
	typedef List<Connection*>::const_iterator ConnectionListIterator;
	
	// cerr << "Preparing disconnection event...\n";
	
	if (_lookup) {
		_patch = _engine.object_store()->find_patch(_node_path.parent());
	
		if (_patch == NULL) {
			_succeeded = false;
			QueuedEvent::pre_process();
			return;
		}
		
		_node = _engine.object_store()->find_node(_node_path);
		
		if (_node == NULL) {
			_succeeded = false;
			QueuedEvent::pre_process();
			return;
		}
	}

	Connection* c = NULL;
	for (ConnectionListIterator i = _patch->connections().begin(); i != _patch->connections().end(); ++i) {
		c = (*i);
		if ((c->src_port()->parent_node() == _node || c->dst_port()->parent_node() == _node) && !c->pending_disconnection()) {
			DisconnectionEvent* ev = new DisconnectionEvent(_engine, SharedPtr<Responder>(new Responder()), _time,
				c->src_port(), c->dst_port());
			ev->pre_process();
			_disconnection_events.push_back(new ListNode<DisconnectionEvent*>(ev));
			c->pending_disconnection(true);
		}
	}
	
	_succeeded = true;
	QueuedEvent::pre_process();	
}


void
DisconnectNodeEvent::execute(SampleCount nframes, FrameTime start, FrameTime end)
{
	QueuedEvent::execute(nframes, start, end);

	if (_succeeded) {
		for (List<DisconnectionEvent*>::iterator i = _disconnection_events.begin(); i != _disconnection_events.end(); ++i)
			(*i)->execute(nframes, start, end);
	}
}


void
DisconnectNodeEvent::post_process()
{
	if (_succeeded) {
		if (_responder)
			_responder->respond_ok();
		for (List<DisconnectionEvent*>::iterator i = _disconnection_events.begin(); i != _disconnection_events.end(); ++i)
			(*i)->post_process();
	} else {
		if (_responder)
			_responder->respond_error("Unable to disconnect all ports.");
	}
}


} // namespace Ingen

