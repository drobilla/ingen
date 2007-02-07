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

#include "ConnectionEvent.h"
#include <string>
#include "Responder.h"
#include "types.h"
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


//// ConnectionEvent ////


ConnectionEvent::ConnectionEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& src_port_path, const string& dst_port_path)
: QueuedEvent(engine, responder, timestamp),
  _src_port_path(src_port_path),
  _dst_port_path(dst_port_path),
  _patch(NULL),
  _src_port(NULL),
  _dst_port(NULL),
  _typed_event(NULL),
  _error(NO_ERROR)
{
}


ConnectionEvent::~ConnectionEvent()
{
	delete _typed_event;
}


void
ConnectionEvent::pre_process()
{
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
	
	/*if (port1->is_output() && port2->is_input()) {
		_src_port = port1;
		_dst_port = port2;
	} else if (port2->is_output() && port1->is_input()) {
		_src_port = port2;
		_dst_port = port1;
	} else {
		_error = TYPE_MISMATCH;
		QueuedEvent::pre_process();
		return;
	}*/
	
	// Create the typed event to actually do the work
	const DataType type = _src_port->type();
	if (type == DataType::FLOAT) {
		_typed_event = new TypedConnectionEvent<Sample>(_engine, _responder, _time,
			dynamic_cast<OutputPort<Sample>*>(_src_port), dynamic_cast<InputPort<Sample>*>(_dst_port));
	} else if (type == DataType::MIDI) {
		_typed_event = new TypedConnectionEvent<MidiMessage>(_engine, _responder, _time,
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
ConnectionEvent::execute(SampleCount nframes, FrameTime start, FrameTime end)
{
	QueuedEvent::execute(nframes, start, end);

	if (_error == NO_ERROR)
		_typed_event->execute(nframes, start, end);
}


void
ConnectionEvent::post_process()
{
	if (_error == NO_ERROR) {
		_typed_event->post_process();
	} else {
		// FIXME: better error messages
		string msg = "Unable to make connection ";
		msg.append(_src_port_path + " -> " + _dst_port_path);
		_responder->respond_error(msg);
	}
}



//// TypedConnectionEvent ////


template <typename T>
TypedConnectionEvent<T>::TypedConnectionEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, OutputPort<T>* src_port, InputPort<T>* dst_port)
: QueuedEvent(engine, responder, timestamp),
  _src_port(src_port),
  _dst_port(dst_port),
  _patch(NULL),
  _process_order(NULL),
  _connection(NULL),
  _port_listnode(NULL),
  _succeeded(true)
{
	assert(src_port != NULL);
	assert(dst_port != NULL);
}


template <typename T>
void
TypedConnectionEvent<T>::pre_process()
{	
	if (_dst_port->is_connected_to(_src_port)) {
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
	
	if (_patch != src_node && src_node->parent() != _patch && dst_node->parent() != _patch) {
		_succeeded = false;
		QueuedEvent::pre_process();
		return;
	}

	_connection = new TypedConnection<T>(_src_port, _dst_port);
	_port_listnode = new ListNode<TypedConnection<T>*>(_connection);
	_patch_listnode = new ListNode<Connection*>(_connection);
	
	// Need to be careful about patch port connections here and adding a node's
	// parent as a dependant/provider, or adding a patch as it's own provider...
	if (src_node != dst_node && src_node->parent() == dst_node->parent()) {
		dst_node->providers()->push_back(new ListNode<Node*>(src_node));
		src_node->dependants()->push_back(new ListNode<Node*>(dst_node));
	}

	if (_patch->enabled())
		_process_order = _patch->build_process_order();

	_succeeded = true;
	QueuedEvent::pre_process();
}


template <typename T>
void
TypedConnectionEvent<T>::execute(SampleCount nframes, FrameTime start, FrameTime end)
{
	QueuedEvent::execute(nframes, start, end);

	if (_succeeded) {
		// These must be inserted here, since they're actually used by the audio thread
		_dst_port->add_connection(_port_listnode);
		_patch->add_connection(_patch_listnode);
		if (_patch->process_order() != NULL)
			_engine.maid()->push(_patch->process_order());
		_patch->process_order(_process_order);
	}
}


template <typename T>
void
TypedConnectionEvent<T>::post_process()
{
	if (_succeeded) {
		assert(_connection != NULL);
	
		_responder->respond_ok();
	
		_engine.broadcaster()->send_connection(_connection);
	} else {
		_responder->respond_error("Unable to make connection.");
	}
}


} // namespace Ingen

