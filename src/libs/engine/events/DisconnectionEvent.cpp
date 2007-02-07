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

#include "DisconnectionEvent.h"
#include <string>
#include "Responder.h"
#include "Engine.h"
#include "TypedConnection.h"
#include "InputPort.h"
#include "OutputPort.h"
#include "Patch.h"
#include "ClientBroadcaster.h"
#include "Port.h"
#include "Maid.h"
#include "ObjectStore.h"
#include "raul/Path.h"

using std::string;
namespace Ingen {


//// DisconnectionEvent ////


DisconnectionEvent::DisconnectionEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& src_port_path, const string& dst_port_path)
: QueuedEvent(engine, responder, timestamp),
  _src_port_path(src_port_path),
  _dst_port_path(dst_port_path),
  _patch(NULL),
  _src_port(NULL),
  _dst_port(NULL),
  _lookup(true),
  _typed_event(NULL),
  _error(NO_ERROR)
{
}


DisconnectionEvent::DisconnectionEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, Port* const src_port, Port* const dst_port)
: QueuedEvent(engine, responder, timestamp),
  _src_port_path(src_port->path()),
  _dst_port_path(dst_port->path()),
  _patch(src_port->parent_node()->parent_patch()),
  _src_port(src_port),
  _dst_port(dst_port),
  _lookup(false),
  _typed_event(NULL),
  _error(NO_ERROR)
{
	// FIXME: These break for patch ports.. is that ok?
	/*assert(src_port->is_output());
	assert(dst_port->is_input());
	assert(src_port->type() == dst_port->type());
	assert(src_port->parent_node()->parent_patch()
			== dst_port->parent_node()->parent_patch()); */
}

DisconnectionEvent::~DisconnectionEvent()
{
	delete _typed_event;
}


void
DisconnectionEvent::pre_process()
{
	if (_lookup) {
		if (_src_port_path.parent().parent() != _dst_port_path.parent().parent()
				&& _src_port_path.parent() != _dst_port_path.parent().parent()
				&& _src_port_path.parent().parent() != _dst_port_path.parent()) {
			_error = PARENT_PATCH_DIFFERENT;
			QueuedEvent::pre_process();
			return;
		}

		/*_patch = _engine.object_store()->find_patch(_src_port_path.parent().parent());

		  if (_patch == NULL) {
		  _error = PORT_NOT_FOUND;
		  QueuedEvent::pre_process();
		  return;
		  }*/

		_src_port = _engine.object_store()->find_port(_src_port_path);
		_dst_port = _engine.object_store()->find_port(_dst_port_path);
	}
	
	if (_src_port == NULL || _dst_port == NULL) {
		_error = PORT_NOT_FOUND;
		QueuedEvent::pre_process();
		return;
	}

	if (_src_port->type() != _dst_port->type() || _src_port->buffer_size() != _dst_port->buffer_size()) {
		_error = TYPE_MISMATCH;
		QueuedEvent::pre_process();
		return;
	}

	// Create the typed event to actually do the work
	const DataType type = _src_port->type();
	if (type == DataType::FLOAT) {
		_typed_event = new TypedDisconnectionEvent<Sample>(_engine, _responder, _time,
				dynamic_cast<OutputPort<Sample>*>(_src_port), dynamic_cast<InputPort<Sample>*>(_dst_port));
	} else if (type == DataType::MIDI) {
		_typed_event = new TypedDisconnectionEvent<MidiMessage>(_engine, _responder, _time,
				dynamic_cast<OutputPort<MidiMessage>*>(_src_port), dynamic_cast<InputPort<MidiMessage>*>(_dst_port));
	} else {
		_error = TYPE_MISMATCH;
		QueuedEvent::pre_process();
		return;
	}

	assert(_typed_event);
	_typed_event->pre_process();
	assert(_typed_event->is_prepared());

	QueuedEvent::pre_process();
}


void
DisconnectionEvent::execute(SampleCount nframes, FrameTime start, FrameTime end)
{
	QueuedEvent::execute(nframes, start, end);

	if (_error == NO_ERROR)
		_typed_event->execute(nframes, start, end);
}


void
DisconnectionEvent::post_process()
{
	if (_error == NO_ERROR) {
		_typed_event->post_process();
	} else {
		// FIXME: better error messages
		string msg = "Unable to disconnect ";
		msg.append(_src_port_path + " -> " + _dst_port_path);
		_responder->respond_error(msg);
	}
}



//// TypedDisconnectionEvent ////


template <typename T>
TypedDisconnectionEvent<T>::TypedDisconnectionEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, OutputPort<T>* src_port, InputPort<T>* dst_port)
: QueuedEvent(engine, responder, timestamp),
  _src_port(src_port),
  _dst_port(dst_port),
  _patch(NULL),
  _process_order(NULL),
  _succeeded(true)
{
	assert(src_port != NULL);
	assert(dst_port != NULL);
}


template <typename T>
void
TypedDisconnectionEvent<T>::pre_process()
{
	if (!_dst_port->is_connected_to(_src_port)) {
		_succeeded = false;
		QueuedEvent::pre_process();
		return;
	}
	
	Node* const src_node = _src_port->parent_node();
	Node* const dst_node = _dst_port->parent_node();

	// Connection to a patch port from inside the patch
	if (src_node->parent_patch() != dst_node->parent_patch()) {

		assert(src_node->parent() == dst_node || dst_node->parent() == src_node);
		if (src_node->parent() == dst_node)
			_patch = dynamic_cast<Patch*>(dst_node);
		else
			_patch = dynamic_cast<Patch*>(src_node);
	
	// Connection from a patch input to a patch output (pass through)
	} else if (src_node == dst_node && dynamic_cast<Patch*>(src_node)) {
		_patch = dynamic_cast<Patch*>(src_node);
	
	// Normal connection between nodes with the same parent
	} else {
		_patch = src_node->parent_patch();
	}

	assert(_patch);

	if (src_node == NULL || dst_node == NULL) {
		_succeeded = false;
		QueuedEvent::pre_process();
		return;
	}
	
	for (List<Node*>::iterator i = dst_node->providers()->begin(); i != dst_node->providers()->end(); ++i)
		if ((*i) == src_node) {
			delete dst_node->providers()->remove(i);
			break;
		}

	for (List<Node*>::iterator i = src_node->dependants()->begin(); i != src_node->dependants()->end(); ++i)
		if ((*i) == dst_node) {
			delete src_node->dependants()->remove(i);
			break;
		}
	
	if (_patch->enabled())
		_process_order = _patch->build_process_order();
	
	_succeeded = true;
	QueuedEvent::pre_process();	
}


template <typename T>
void
TypedDisconnectionEvent<T>::execute(SampleCount nframes, FrameTime start, FrameTime end)
{
	QueuedEvent::execute(nframes, start, end);

	if (_succeeded) {

		ListNode<TypedConnection<T>*>* const port_connection
			= _dst_port->remove_connection(_src_port);
		
		if (port_connection != NULL) {
			ListNode<Connection*>* const patch_connection
				= _patch->remove_connection(_src_port, _dst_port);
			
			assert(patch_connection);
			assert((Connection*)port_connection->elem() == patch_connection->elem());
			
			// Clean up both the list node and the connection itself...
			_engine.maid()->push(port_connection);
			_engine.maid()->push(patch_connection);
			_engine.maid()->push(port_connection->elem());
	
			if (_patch->process_order() != NULL)
				_engine.maid()->push(_patch->process_order());
			_patch->process_order(_process_order);
		} else {
			_succeeded = false;  // Ports weren't connected
		}
	}
}


template <typename T>
void
TypedDisconnectionEvent<T>::post_process()
{
	if (_succeeded) {
	
		_responder->respond_ok();
	
		_engine.broadcaster()->send_disconnection(_src_port->path(), _dst_port->path());
	} else {
		_responder->respond_error("Unable to disconnect ports.");
	}
}


} // namespace Ingen

