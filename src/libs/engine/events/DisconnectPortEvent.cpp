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

#include <iostream>
#include <raul/Maid.hpp>
#include <raul/List.hpp>
#include <raul/Path.hpp>
#include <raul/Array.hpp>
#include "Responder.hpp"
#include "Engine.hpp"
#include "NodeImpl.hpp"
#include "ConnectionImpl.hpp"
#include "DisconnectionEvent.hpp"
#include "PortImpl.hpp"
#include "InputPort.hpp"
#include "OutputPort.hpp"
#include "Patch.hpp"
#include "ClientBroadcaster.hpp"
#include "util.hpp"
#include "ObjectStore.hpp"
#include "DisconnectPortEvent.hpp"

using std::cerr; using std::endl;

namespace Ingen {


DisconnectPortEvent::DisconnectPortEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& port_path)
: QueuedEvent(engine, responder, timestamp),
  _port_path(port_path),
  _patch(NULL),
  _port(NULL),
  _process_order(NULL),
  _succeeded(true),
  _lookup(true)
{
}


DisconnectPortEvent::DisconnectPortEvent(Engine& engine, Patch* patch, Port* port)
: QueuedEvent(engine),
  _port_path(port->path()),
  _patch(patch),
  _port(port),
  _process_order(NULL),
  _succeeded(true),
  _lookup(false)
{
	//cerr << "DisconnectPortEvent(Engine& engine, )\n";
}


DisconnectPortEvent::~DisconnectPortEvent()
{
	for (Raul::List<DisconnectionEvent*>::iterator i = _disconnection_events.begin(); i != _disconnection_events.end(); ++i)
		delete (*i);
}


void
DisconnectPortEvent::pre_process()
{
	// cerr << "Preparing disconnection event...\n";
	
	if (_lookup) {

		/* FIXME: Double lookup */

		_patch = _engine.object_store()->find_patch(_port_path.parent().parent());
	
		if (_patch == NULL) {
			_succeeded = false;
			QueuedEvent::pre_process();
			return;
		}
		
		_port = _engine.object_store()->find_port(_port_path);
		
		if (_port == NULL) {
			_succeeded = false;
			QueuedEvent::pre_process();
			return;
		}
	}

	if (_patch == NULL) {
		_succeeded = false;
		QueuedEvent::pre_process();
		return;
	}
	
	for (Patch::Connections::const_iterator i = _patch->connections().begin();
			i != _patch->connections().end(); ++i) {
		const SharedPtr<ConnectionImpl> c(*i);
		if ((c->src_port() == _port || c->dst_port() == _port) && !c->pending_disconnection()) {
			DisconnectionEvent* ev = new DisconnectionEvent(_engine, SharedPtr<Responder>(new Responder()), _time,
					c->src_port(), c->dst_port());
			ev->pre_process();
			_disconnection_events.push_back(new Raul::List<DisconnectionEvent*>::Node(ev));
			c->pending_disconnection(true);
		}
	}

	_succeeded = true;
	QueuedEvent::pre_process();	
}


void
DisconnectPortEvent::execute(ProcessContext& context)
{
	QueuedEvent::execute(context);

	if (_succeeded) {
		for (Raul::List<DisconnectionEvent*>::iterator i = _disconnection_events.begin(); i != _disconnection_events.end(); ++i)
			(*i)->execute(context);
	}
}


void
DisconnectPortEvent::post_process()
{
	if (_succeeded) {
		if (_responder)
			_responder->respond_ok();
		for (Raul::List<DisconnectionEvent*>::iterator i = _disconnection_events.begin(); i != _disconnection_events.end(); ++i)
			(*i)->post_process();
	} else {
		if (_responder)
			_responder->respond_error("Unable to disconnect port.");
	}
}


} // namespace Ingen

