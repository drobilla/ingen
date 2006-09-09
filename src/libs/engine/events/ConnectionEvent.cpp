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
#include "util/Path.h"

using std::string;
namespace Ingen {


//// ConnectionEvent ////


ConnectionEvent::ConnectionEvent(Engine& engine, CountedPtr<Responder> responder, SampleCount timestamp, const string& src_port_path, const string& dst_port_path)
: QueuedEvent(engine, responder, timestamp),
  m_src_port_path(src_port_path),
  m_dst_port_path(dst_port_path),
  m_patch(NULL),
  m_src_port(NULL),
  m_dst_port(NULL),
  m_typed_event(NULL),
  m_error(NO_ERROR)
{
}


ConnectionEvent::~ConnectionEvent()
{
	delete m_typed_event;
}


void
ConnectionEvent::pre_process()
{
	if (m_src_port_path.parent().parent() != m_dst_port_path.parent().parent()
			&& m_src_port_path.parent() != m_dst_port_path.parent().parent()
			&& m_src_port_path.parent().parent() != m_dst_port_path.parent()) {
		m_error = PARENT_PATCH_DIFFERENT;
		QueuedEvent::pre_process();
		return;
	}
	
	/*m_patch = _engine.object_store()->find_patch(m_src_port_path.parent().parent());

	if (m_patch == NULL) {
		m_error = PORT_NOT_FOUND;
		QueuedEvent::pre_process();
		return;
	}*/
	
	m_src_port = _engine.object_store()->find_port(m_src_port_path);
	m_dst_port = _engine.object_store()->find_port(m_dst_port_path);
	
	if (m_src_port == NULL || m_dst_port == NULL) {
		m_error = PORT_NOT_FOUND;
		QueuedEvent::pre_process();
		return;
	}

	if (m_src_port->type() != m_dst_port->type() || m_src_port->buffer_size() != m_dst_port->buffer_size()) {
		m_error = TYPE_MISMATCH;
		QueuedEvent::pre_process();
		return;
	}
	
	/*if (port1->is_output() && port2->is_input()) {
		m_src_port = port1;
		m_dst_port = port2;
	} else if (port2->is_output() && port1->is_input()) {
		m_src_port = port2;
		m_dst_port = port1;
	} else {
		m_error = TYPE_MISMATCH;
		QueuedEvent::pre_process();
		return;
	}*/
	
	// Create the typed event to actually do the work
	const DataType type = m_src_port->type();
	if (type == DataType::FLOAT) {
		m_typed_event = new TypedConnectionEvent<Sample>(_engine, _responder, _time,
			dynamic_cast<OutputPort<Sample>*>(m_src_port), dynamic_cast<InputPort<Sample>*>(m_dst_port));
	} else if (type == DataType::MIDI) {
		m_typed_event = new TypedConnectionEvent<MidiMessage>(_engine, _responder, _time,
			dynamic_cast<OutputPort<MidiMessage>*>(m_src_port), dynamic_cast<InputPort<MidiMessage>*>(m_dst_port));
	} else {
		m_error = TYPE_MISMATCH;
		QueuedEvent::pre_process();
		return;
	}

	m_typed_event->pre_process();
	
	QueuedEvent::pre_process();
}


void
ConnectionEvent::execute(SampleCount nframes, FrameTime start, FrameTime end)
{
	QueuedEvent::execute(nframes, start, end);

	if (m_error == NO_ERROR)
		m_typed_event->execute(nframes, start, end);
}


void
ConnectionEvent::post_process()
{
	if (m_error == NO_ERROR) {
		m_typed_event->post_process();
	} else {
		// FIXME: better error messages
		string msg = "Unable to make connection ";
		msg.append(m_src_port_path + " -> " + m_dst_port_path);
		_responder->respond_error(msg);
	}
}



//// TypedConnectionEvent ////


template <typename T>
TypedConnectionEvent<T>::TypedConnectionEvent(Engine& engine, CountedPtr<Responder> responder, SampleCount timestamp, OutputPort<T>* src_port, InputPort<T>* dst_port)
: QueuedEvent(engine, responder, timestamp),
  m_src_port(src_port),
  m_dst_port(dst_port),
  m_patch(NULL),
  m_process_order(NULL),
  m_connection(NULL),
  m_port_listnode(NULL),
  m_succeeded(true)
{
	assert(src_port != NULL);
	assert(dst_port != NULL);
}


template <typename T>
void
TypedConnectionEvent<T>::pre_process()
{	
	if (m_dst_port->is_connected_to(m_src_port)) {
		m_succeeded = false;
		QueuedEvent::pre_process();
		return;
	}

	Node* const src_node = m_src_port->parent_node();
	Node* const dst_node = m_dst_port->parent_node();

	// Connection to a patch port from inside the patch
	if (src_node->parent_patch() != dst_node->parent_patch()) {

		assert(src_node->parent() == dst_node || dst_node->parent() == src_node);
		if (src_node->parent() == dst_node)
			m_patch = dynamic_cast<Patch*>(dst_node);
		else
			m_patch = dynamic_cast<Patch*>(src_node);
	
	// Connection from a patch input to a patch output (pass through)
	} else if (src_node == dst_node && dynamic_cast<Patch*>(src_node)) {
		m_patch = dynamic_cast<Patch*>(src_node);
	
	// Normal connection between nodes with the same parent
	} else {
		m_patch = src_node->parent_patch();
	}

	assert(m_patch);

	if (src_node == NULL || dst_node == NULL) {
		m_succeeded = false;
		QueuedEvent::pre_process();
		return;
	}
	
	if (m_patch != src_node && src_node->parent() != m_patch && dst_node->parent() != m_patch) {
		m_succeeded = false;
		QueuedEvent::pre_process();
		return;
	}

	m_connection = new TypedConnection<T>(m_src_port, m_dst_port);
	m_port_listnode = new ListNode<TypedConnection<T>*>(m_connection);
	m_patch_listnode = new ListNode<Connection*>(m_connection);
	
	// Need to be careful about patch port connections here and adding a node's
	// parent as a dependant/provider, or adding a patch as it's own provider...
	if (src_node != dst_node && src_node->parent() == dst_node->parent()) {
		dst_node->providers()->push_back(new ListNode<Node*>(src_node));
		src_node->dependants()->push_back(new ListNode<Node*>(dst_node));
	}

	if (m_patch->enabled())
		m_process_order = m_patch->build_process_order();
}


template <typename T>
void
TypedConnectionEvent<T>::execute(SampleCount nframes, FrameTime start, FrameTime end)
{
	if (m_succeeded) {
		// These must be inserted here, since they're actually used by the audio thread
		m_dst_port->add_connection(m_port_listnode);
		m_patch->add_connection(m_patch_listnode);
		if (m_patch->process_order() != NULL)
			_engine.maid()->push(m_patch->process_order());
		m_patch->process_order(m_process_order);
	}
}


template <typename T>
void
TypedConnectionEvent<T>::post_process()
{
	if (m_succeeded) {
		assert(m_connection != NULL);
	
		_responder->respond_ok();
	
		_engine.broadcaster()->send_connection(m_connection);
	} else {
		_responder->respond_error("Unable to make connection.");
	}
}


} // namespace Ingen

