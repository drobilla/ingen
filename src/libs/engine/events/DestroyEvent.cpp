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

#include "DestroyEvent.hpp"
#include "Responder.hpp"
#include "Engine.hpp"
#include "Patch.hpp"
#include "Tree.hpp"
#include "NodeBase.hpp"
#include "Plugin.hpp"
#include "AudioDriver.hpp"
#include "MidiDriver.hpp"
#include "DisconnectNodeEvent.hpp"
#include "DisconnectPortEvent.hpp"
#include "ClientBroadcaster.hpp"
#include <raul/Maid.hpp>
#include "ObjectStore.hpp"
#include <raul/Path.hpp>
#include "QueuedEventSource.hpp"
#include "Port.hpp"

namespace Ingen {


DestroyEvent::DestroyEvent(Engine& engine, SharedPtr<Responder> responder, FrameTime time, QueuedEventSource* source, const string& path, bool block)
: QueuedEvent(engine, responder, time, source, source),
  _path(path),
  _store_iterator(engine.object_store()->objects().end()),
  _removed_table(NULL),
  _node(NULL),
  _port(NULL),
  _driver_port(NULL),
  _patch_node_listnode(NULL),
  _patch_port_listnode(NULL),
  _ports_array(NULL),
  _compiled_patch(NULL),
  _disconnect_node_event(NULL),
  _disconnect_port_event(NULL)
{
	assert(_source);
}


DestroyEvent::~DestroyEvent()
{
	delete _disconnect_node_event;
	delete _disconnect_port_event;
}


void
DestroyEvent::pre_process()
{
	_store_iterator = _engine.object_store()->find(_path);

	if (_store_iterator != _engine.object_store()->objects().end())  {
		_node = dynamic_cast<Node*>(_store_iterator->second);

		if (!_node)
			_port = dynamic_cast<Port*>(_store_iterator->second);
	}
			
	if (_store_iterator != _engine.object_store()->objects().end()) {
		_removed_table = _engine.object_store()->remove(_store_iterator);
	}

	if (_node != NULL && _path != "/") {
		assert(_node->parent_patch());
		_patch_node_listnode = _node->parent_patch()->remove_node(_path.name());
		if (_patch_node_listnode) {
			assert(_patch_node_listnode->elem() == _node);
			
			_disconnect_node_event = new DisconnectNodeEvent(_engine, _node);
			_disconnect_node_event->pre_process();
			
			if (_node->parent_patch()->enabled()) {
				// FIXME: is this called multiple times?
				_compiled_patch = _node->parent_patch()->compile();
#ifndef NDEBUG
				// Be sure node is removed from process order, so it can be destroyed
				for (size_t i=0; i < _compiled_patch->size(); ++i) {
					assert(_compiled_patch->at(i).node() != _node);
					// FIXME: check providers/dependants too
				}
#endif
			}
		}
	} else if (_port) {
		assert(_port->parent_patch());
		_patch_port_listnode = _port->parent_patch()->remove_port(_path.name());
		if (_patch_port_listnode) {
			assert(_patch_port_listnode->elem() == _port);
			
			//_port->remove_from_store();

			_disconnect_port_event = new DisconnectPortEvent(_engine, _port->parent_patch(), _port);
			_disconnect_port_event->pre_process();
			
			if (_port->parent_patch()->enabled()) {
				// FIXME: is this called multiple times?
				_compiled_patch = _port->parent_patch()->compile();
				_ports_array   = _port->parent_patch()->build_ports_array();
				assert(_ports_array->size() == _port->parent_patch()->num_ports());
			}
		}

	}

	QueuedEvent::pre_process();
}


void
DestroyEvent::execute(ProcessContext& context)
{
	QueuedEvent::execute(context);

	if (_patch_node_listnode) {
		assert(_node);

		if (_disconnect_node_event)
			_disconnect_node_event->execute(context);
		
		if (_node->parent_patch()->compiled_patch())
			_engine.maid()->push(_node->parent_patch()->compiled_patch());
		_node->parent_patch()->compiled_patch(_compiled_patch);
	
	} else if (_patch_port_listnode) {
		assert(_port);

		if (_disconnect_port_event)
			_disconnect_port_event->execute(context);
		
		if (_port->parent_patch()->compiled_patch())
			_engine.maid()->push(_port->parent_patch()->compiled_patch());
		
		_port->parent_patch()->compiled_patch(_compiled_patch);
		
		if (_port->parent_patch()->external_ports())
			_engine.maid()->push(_port->parent_patch()->external_ports());
		
		_port->parent_patch()->external_ports(_ports_array);
		
		if ( ! _port->parent_patch()->parent()) {
			if (_port->type() == DataType::FLOAT)
				_driver_port = _engine.audio_driver()->remove_port(_port->path());
			else if (_port->type() == DataType::MIDI)
				_driver_port = _engine.midi_driver()->remove_port(_port->path());
		}
	}
	
	if (_source)
		_source->unblock();
}


void
DestroyEvent::post_process()
{
	if (_store_iterator != _engine.object_store()->objects().end()) {
		_engine.broadcaster()->send_destroyed(_path);
	} else {
		if (_path == "/")
			_responder->respond_error("You can not destroy the root patch (/)");
		else
			_responder->respond_error("Could not find object to destroy");
	}

	if (_patch_node_listnode) {	
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
}


} // namespace Ingen
