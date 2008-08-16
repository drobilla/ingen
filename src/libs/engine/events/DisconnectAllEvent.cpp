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

#include <boost/format.hpp>
#include <raul/Array.hpp>
#include <raul/List.hpp>
#include <raul/Maid.hpp>
#include <raul/Path.hpp>
#include "ClientBroadcaster.hpp"
#include "ConnectionImpl.hpp"
#include "DisconnectAllEvent.hpp"
#include "DisconnectionEvent.hpp"
#include "Engine.hpp"
#include "InputPort.hpp"
#include "NodeImpl.hpp"
#include "EngineStore.hpp"
#include "OutputPort.hpp"
#include "PatchImpl.hpp"
#include "PortImpl.hpp"
#include "Responder.hpp"
#include "util.hpp"

namespace Ingen {


DisconnectAllEvent::DisconnectAllEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& parent_path, const string& node_path)
	: QueuedEvent(engine, responder, timestamp)
	, _parent_path(parent_path)
	, _path(node_path)
	, _parent(NULL)
	, _node(NULL)
	, _port(NULL)
	, _lookup(true)
	, _error(NO_ERROR)
{
}


/** Internal version for use by other events.
 */
DisconnectAllEvent::DisconnectAllEvent(Engine& engine, PatchImpl* parent, GraphObjectImpl* object)
	: QueuedEvent(engine)
	, _parent_path(parent->path())
	, _path(object->path())
	, _parent(parent)
	, _node(dynamic_cast<NodeImpl*>(object))
	, _port(dynamic_cast<PortImpl*>(object))
	, _lookup(false)
	, _error(NO_ERROR)
{
}


DisconnectAllEvent::~DisconnectAllEvent()
{
	for (Raul::List<DisconnectionEvent*>::iterator i = _disconnection_events.begin(); i != _disconnection_events.end(); ++i)
		delete (*i);
}


void
DisconnectAllEvent::pre_process()
{
	if (_lookup) {
		_parent = _engine.engine_store()->find_patch(_parent_path);
	
		if (_parent == NULL) {
			_error = PARENT_NOT_FOUND;
			QueuedEvent::pre_process();
			return;
		}
		
		GraphObjectImpl* object = _engine.engine_store()->find_object(_path);
		
		if (object == NULL) {
			_error = OBJECT_NOT_FOUND;
			QueuedEvent::pre_process();
			return;
		}
		
		if (object->parent_patch() != _parent && object->parent()->parent_patch() != _parent) {
			_error = INVALID_PARENT_PATH;
			QueuedEvent::pre_process();
			return;
		}

		// Only one of these will succeed
		_node = dynamic_cast<NodeImpl*>(object);
		_port = dynamic_cast<PortImpl*>(object);

		assert((_node || _port) && !(_node && _port));
	}

	if (_node) {
		for (PatchImpl::Connections::const_iterator i = _parent->connections().begin();
				i != _parent->connections().end(); ++i) {
			ConnectionImpl* c = (ConnectionImpl*)i->get();
			if ((c->src_port()->parent_node() == _node || c->dst_port()->parent_node() == _node)
					&& !c->pending_disconnection()) {
				DisconnectionEvent* ev = new DisconnectionEvent(_engine,
						SharedPtr<Responder>(new Responder()), _time, c->src_port(), c->dst_port());
				ev->pre_process();
				_disconnection_events.push_back(new Raul::List<DisconnectionEvent*>::Node(ev));
				c->pending_disconnection(true);
			}
		}
	} else { // _port
		for (PatchImpl::Connections::const_iterator i = _parent->connections().begin();
				i != _parent->connections().end(); ++i) {
			ConnectionImpl* c = (ConnectionImpl*)i->get();
			if ((c->src_port() == _port || c->dst_port() == _port) && !c->pending_disconnection()) {
				DisconnectionEvent* ev = new DisconnectionEvent(_engine,
						SharedPtr<Responder>(new Responder()), _time, c->src_port(), c->dst_port());
				ev->pre_process();
				_disconnection_events.push_back(new Raul::List<DisconnectionEvent*>::Node(ev));
				c->pending_disconnection(true);
			}
		}
	}
	
	QueuedEvent::pre_process();	
}


void
DisconnectAllEvent::execute(ProcessContext& context)
{
	QueuedEvent::execute(context);

	if (_error == NO_ERROR) {
		for (Raul::List<DisconnectionEvent*>::iterator i = _disconnection_events.begin(); i != _disconnection_events.end(); ++i)
			(*i)->execute(context);
	}
}


void
DisconnectAllEvent::post_process()
{
	if (_error == NO_ERROR) {
		if (_responder)
			_responder->respond_ok();
		for (Raul::List<DisconnectionEvent*>::iterator i = _disconnection_events.begin();
				i != _disconnection_events.end(); ++i)
			(*i)->post_process();
	} else {
		if (_responder) {
			boost::format fmt("Unable to disconnect %1% (%2%)");
			fmt % _path;
			switch (_error) {
				case INVALID_PARENT_PATH:
					fmt % string("Invalid parent path: ").append(_parent_path);
					break;
				case PARENT_NOT_FOUND:
					fmt % string("Unable to find parent: ").append(_parent_path);
					break;
				case OBJECT_NOT_FOUND:
					fmt % string("Unable to find object");
				default:
					break;
			}
			_responder->respond_error(fmt.str());
		}
	}
}


} // namespace Ingen

