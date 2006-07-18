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

#include "DisconnectionEvent.h"
#include <string>
#include "Responder.h"
#include "Ingen.h"
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
namespace Om {


//// DisconnectionEvent ////


DisconnectionEvent::DisconnectionEvent(CountedPtr<Responder> responder, SampleCount timestamp, const string& src_port_path, const string& dst_port_path)
: QueuedEvent(responder, timestamp),
  m_src_port_path(src_port_path),
  m_dst_port_path(dst_port_path),
  m_patch(NULL),
  m_src_port(NULL),
  m_dst_port(NULL),
  m_lookup(true),
  m_typed_event(NULL),
  m_error(NO_ERROR)
{
}


DisconnectionEvent::DisconnectionEvent(CountedPtr<Responder> responder, SampleCount timestamp, Port* const src_port, Port* const dst_port)
: QueuedEvent(responder, timestamp),
  m_src_port_path(src_port->path()),
  m_dst_port_path(dst_port->path()),
  m_patch(src_port->parent_node()->parent_patch()),
  m_src_port(src_port),
  m_dst_port(dst_port),
  m_lookup(false),
  m_typed_event(NULL),
  m_error(NO_ERROR)
{
	assert(src_port->is_output());
	assert(dst_port->is_input());
	assert(src_port->type() == dst_port->type());
	assert(src_port->parent_node()->parent_patch()
			== dst_port->parent_node()->parent_patch());
}

DisconnectionEvent::~DisconnectionEvent()
{
	delete m_typed_event;
}


void
DisconnectionEvent::pre_process()
{
	if (m_lookup) {
		if (m_src_port_path.parent().parent() != m_dst_port_path.parent().parent()
				&& m_src_port_path.parent() != m_dst_port_path.parent().parent()
				&& m_src_port_path.parent().parent() != m_dst_port_path.parent()) {
			m_error = PARENT_PATCH_DIFFERENT;
			QueuedEvent::pre_process();
			return;
		}

		/*m_patch = Ingen::instance().object_store()->find_patch(m_src_port_path.parent().parent());

		  if (m_patch == NULL) {
		  m_error = PORT_NOT_FOUND;
		  QueuedEvent::pre_process();
		  return;
		  }*/

		m_src_port = Ingen::instance().object_store()->find_port(m_src_port_path);
		m_dst_port = Ingen::instance().object_store()->find_port(m_dst_port_path);
	}
	
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

	// Create the typed event to actually do the work
	const DataType type = m_src_port->type();
	if (type == DataType::FLOAT) {
		m_typed_event = new TypedDisconnectionEvent<Sample>(_responder, _time_stamp,
				dynamic_cast<OutputPort<Sample>*>(m_src_port), dynamic_cast<InputPort<Sample>*>(m_dst_port));
	} else if (type == DataType::MIDI) {
		m_typed_event = new TypedDisconnectionEvent<MidiMessage>(_responder, _time_stamp,
				dynamic_cast<OutputPort<MidiMessage>*>(m_src_port), dynamic_cast<InputPort<MidiMessage>*>(m_dst_port));
	} else {
		m_error = TYPE_MISMATCH;
		QueuedEvent::pre_process();
		return;
	}

	assert(m_typed_event);
	m_typed_event->pre_process();

	QueuedEvent::pre_process();
}


void
DisconnectionEvent::execute(SampleCount offset)
{
	QueuedEvent::execute(offset);

	if (m_error == NO_ERROR)
		m_typed_event->execute(offset);
}


void
DisconnectionEvent::post_process()
{
	if (m_error == NO_ERROR) {
		m_typed_event->post_process();
	} else {
		// FIXME: better error messages
		string msg = "Unable to disconnect ";
		msg.append(m_src_port_path + " -> " + m_dst_port_path);
		_responder->respond_error(msg);
	}
}



//// TypedDisconnectionEvent ////


template <typename T>
TypedDisconnectionEvent<T>::TypedDisconnectionEvent(CountedPtr<Responder> responder, SampleCount timestamp, OutputPort<T>* src_port, InputPort<T>* dst_port)
: QueuedEvent(responder, timestamp),
  m_src_port(src_port),
  m_dst_port(dst_port),
  m_patch(NULL),
  m_process_order(NULL),
  m_succeeded(true)
{
	assert(src_port != NULL);
	assert(dst_port != NULL);
}


template <typename T>
void
TypedDisconnectionEvent<T>::pre_process()
{
	if (!m_dst_port->is_connected_to(m_src_port)) {
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
	
	if (m_patch->process())
		m_process_order = m_patch->build_process_order();
	
	m_succeeded = true;
	QueuedEvent::pre_process();	
}


template <typename T>
void
TypedDisconnectionEvent<T>::execute(SampleCount offset)
{
	if (m_succeeded) {

		ListNode<TypedConnection<T>*>* const port_connection
			= m_dst_port->remove_connection(m_src_port);
		
		if (port_connection != NULL) {
			ListNode<Connection*>* const patch_connection
				= m_patch->remove_connection(m_src_port, m_dst_port);
			
			assert(patch_connection);
			assert((Connection*)port_connection->elem() == patch_connection->elem());
			
			// Clean up both the list node and the connection itself...
			Ingen::instance().maid()->push(port_connection);
			Ingen::instance().maid()->push(patch_connection);
			Ingen::instance().maid()->push(port_connection->elem());
	
			if (m_patch->process_order() != NULL)
				Ingen::instance().maid()->push(m_patch->process_order());
			m_patch->process_order(m_process_order);
		} else {
			m_succeeded = false;  // Ports weren't connected
		}
	}
	QueuedEvent::execute(offset);
}


template <typename T>
void
TypedDisconnectionEvent<T>::post_process()
{
	if (m_succeeded) {
	
		_responder->respond_ok();
	
		Ingen::instance().client_broadcaster()->send_disconnection(m_src_port->path(), m_dst_port->path());
	} else {
		_responder->respond_error("Unable to disconnect ports.");
	}
}


} // namespace Om

