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

#include "DestroyEvent.h"
#include "Responder.h"
#include "Engine.h"
#include "Patch.h"
#include "Tree.h"
#include "Node.h"
#include "Plugin.h"
#include "AudioDriver.h"
#include "MidiDriver.h"
#include "InternalNode.h"
#include "DisconnectNodeEvent.h"
#include "DisconnectPortEvent.h"
#include "ClientBroadcaster.h"
#include <raul/Maid.h>
#include "ObjectStore.h"
#include <raul/Path.h>
#include "QueuedEventSource.h"
#include "Port.h"

namespace Ingen {


DestroyEvent::DestroyEvent(Engine& engine, SharedPtr<Responder> responder, FrameTime time, QueuedEventSource* source, const string& path, bool block)
: QueuedEvent(engine, responder, time, source, source),
  _path(path),
  _object(NULL),
  _node(NULL),
  _port(NULL),
  _driver_port(NULL),
  _patch_node_listnode(NULL),
  _patch_port_listnode(NULL),
  _store_treenode(NULL),
  _ports_array(NULL),
  _process_order(NULL),
  _disconnect_node_event(NULL),
  _disconnect_port_event(NULL)
{
	assert(_source);
}


DestroyEvent::DestroyEvent(Engine& engine, SharedPtr<Responder> responder, FrameTime time, QueuedEventSource* source, Node* node, bool block)
: QueuedEvent(engine, responder, block, source),
  _path(node->path()),
  _object(node),
  _node(node),
  _port(NULL),
  _driver_port(NULL),
  _patch_node_listnode(NULL),
  _patch_port_listnode(NULL),
  _store_treenode(NULL),
  _ports_array(NULL),
  _process_order(NULL),
  _disconnect_node_event(NULL),
  _disconnect_port_event(NULL)
{
}


DestroyEvent::~DestroyEvent()
{
	delete _disconnect_node_event;
	delete _disconnect_port_event;
}


void
DestroyEvent::pre_process()
{
	if (_object == NULL) {
		_object = _engine.object_store()->find(_path);

		if (_object)  {
			_node = dynamic_cast<Node*>(_object);

			if (!_node)
				_port = dynamic_cast<Port*>(_object);
		}
	}

	if (_node != NULL && _path != "/") {
		assert(_node->parent_patch());
		_patch_node_listnode = _node->parent_patch()->remove_node(_path.name());
		if (_patch_node_listnode) {
			assert(_patch_node_listnode->elem() == _node);
			
			_node->remove_from_store();

			_disconnect_node_event = new DisconnectNodeEvent(_engine, _node);
			_disconnect_node_event->pre_process();
			
			if (_node->parent_patch()->enabled()) {
				// FIXME: is this called multiple times?
				_process_order = _node->parent_patch()->build_process_order();
				// Remove node to be removed from the process order so it isn't executed by
				// Patch::run and can safely be destroyed
				//for (size_t i=0; i < _process_order->size(); ++i)
				//	if (_process_order->at(i) == _node)
				//		_process_order->at(i) = NULL; // ew, gap
				
#ifdef DEBUG
				// Be sure node is removed from process order, so it can be destroyed
				for (size_t i=0; i < _process_order->size(); ++i)
					assert(_process_order->at(i) != _node);
#endif
			}
		}
	} else if (_port) {
		assert(_port->parent_patch());
		_patch_port_listnode = _port->parent_patch()->remove_port(_path.name());
		if (_patch_port_listnode) {
			assert(_patch_port_listnode->elem() == _port);
			
			_port->remove_from_store();

			_disconnect_port_event = new DisconnectPortEvent(_engine, _port);
			_disconnect_port_event->pre_process();
			
			if (_port->parent_patch()->enabled()) {
				// FIXME: is this called multiple times?
				_process_order = _port->parent_patch()->build_process_order();
				_ports_array   = _port->parent_patch()->build_ports_array();
				assert(_ports_array->size() == _port->parent_patch()->num_ports());
			}
		}

	}

	QueuedEvent::pre_process();
}


void
DestroyEvent::execute(SampleCount nframes, FrameTime start, FrameTime end)
{
	QueuedEvent::execute(nframes, start, end);

	if (_patch_node_listnode) {
		assert(_node);

		if (_disconnect_node_event)
			_disconnect_node_event->execute(nframes, start, end);
		
		if (_node->parent_patch()->process_order())
			_engine.maid()->push(_node->parent_patch()->process_order());
		_node->parent_patch()->process_order(_process_order);
	
	} else if (_patch_port_listnode) {
		assert(_port);

		if (_disconnect_port_event)
			_disconnect_port_event->execute(nframes, start, end);
		
		if (_port->parent_patch()->process_order())
			_engine.maid()->push(_port->parent_patch()->process_order());
		
		_port->parent_patch()->process_order(_process_order);
		
		if (_port->parent_patch()->external_ports())
			_engine.maid()->push(_port->parent_patch()->external_ports());
		
		_port->parent_patch()->external_ports(_ports_array);
		
		if (!_port->parent_patch()->parent()) {
			_driver_port = _engine.audio_driver()->remove_port(_port->path());
			if (!_driver_port)
				_driver_port = _engine.midi_driver()->remove_port(_port->path());
		}
	}
}


void
DestroyEvent::post_process()
{
	if (_source)
		_source->unblock();
	
	if (_object == NULL) {
		if (_path == "/")
			_responder->respond_error("You can not destroy the root patch (/)");
		else
			_responder->respond_error("Could not find object to destroy");
	} else if (_patch_node_listnode) {	
		assert(_node);
		_node->deactivate();
		_responder->respond_ok();
		if (_disconnect_node_event)
			_disconnect_node_event->post_process();
		_engine.broadcaster()->send_destroyed(_path);
		_engine.maid()->push(_patch_node_listnode);
		_engine.maid()->push(_node);
	} else if (_patch_port_listnode) {	
		assert(_port);
		_responder->respond_ok();
		if (_disconnect_port_event)
			_disconnect_port_event->post_process();
		_engine.broadcaster()->send_destroyed(_path);
		_engine.maid()->push(_patch_port_listnode);
		_engine.maid()->push(_port);
	} else {
		_responder->respond_error("Unable to destroy object");
	}

	if (_driver_port)
		delete _driver_port;
		//_engine.maid()->push(_driver_port);
}


} // namespace Ingen
