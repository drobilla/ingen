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

#include "InputPort.h"
#include <iostream>
#include <cstdlib>
#include <cassert>
#include "TypedConnection.h"
#include "OutputPort.h"
#include "Node.h"
#include "util.h"

using std::cerr; using std::cout; using std::endl;


namespace Ingen {


template <typename T>
InputPort<T>::InputPort(Node* parent, const string& name, size_t index, size_t poly, DataType type, size_t buffer_size)
: TypedPort<T>(parent, name, index, poly, type, buffer_size)
{
}
template InputPort<Sample>::InputPort(Node* parent, const string& name, size_t index, size_t poly, DataType type, size_t buffer_size);
template InputPort<MidiMessage>::InputPort(Node* parent, const string& name, size_t index, size_t poly, DataType type, size_t buffer_size);


/** Add a connection.  Realtime safe.
 *
 * The buffer of this port will be set directly to the connection's buffer
 * if there is only one connection, since no mixing needs to take place.
 */
template<typename T>
void
InputPort<T>::add_connection(ListNode<TypedConnection<T>*>* const c)
{
	m_connections.push_back(c);

	bool modify_buffers = !m_fixed_buffers;
	//if (modify_buffers && m_is_tied)
	//	modify_buffers = !m_tied_port->fixed_buffers();
	
	if (modify_buffers) {
		if (m_connections.size() == 1) {
			// Use buffer directly to avoid copying
			for (size_t i=0; i < _poly; ++i) {
				m_buffers.at(i)->join(c->elem()->buffer(i));
				//if (m_is_tied)
				//	m_tied_port->buffer(i)->join(m_buffers.at(i));
				assert(m_buffers.at(i)->data() == c->elem()->buffer(i)->data());
			}
		} else if (m_connections.size() == 2) {
			// Used to directly use single connection buffer, now there's two
			// so have to use local ones again and mix down
			for (size_t i=0; i < _poly; ++i) {
				m_buffers.at(i)->unjoin();
				//if (m_is_tied)
				//	m_tied_port->buffer(i)->join(m_buffers.at(i));
			}
		}
		TypedPort<T>::connect_buffers();
	}
		
	//assert( ! m_is_tied || m_tied_port != NULL);
	//assert( ! m_is_tied || m_buffers.at(0)->data() == m_tied_port->buffer(0)->data());
}
template void InputPort<Sample>::add_connection(ListNode<TypedConnection<Sample>*>* const c);
template void InputPort<MidiMessage>::add_connection(ListNode<TypedConnection<MidiMessage>*>* const c);


/** Remove a connection.  Realtime safe.
 */
template <typename T>
ListNode<TypedConnection<T>*>*
InputPort<T>::remove_connection(const OutputPort<T>* const src_port)
{
	bool modify_buffers = !m_fixed_buffers;
	//if (modify_buffers && m_is_tied)
	//	modify_buffers = !m_tied_port->fixed_buffers();
	
	typedef typename List<TypedConnection<T>*>::iterator TypedConnectionListIterator;
	bool found = false;
	ListNode<TypedConnection<T>*>* connection = NULL;
	for (TypedConnectionListIterator i = m_connections.begin(); i != m_connections.end(); ++i) {
		if ((*i)->src_port()->path() == src_port->path()) {
			connection = m_connections.remove(i);
			found = true;
		}
	}

	if ( ! found) {
		cerr << "WARNING:  [InputPort<T>::remove_connection] Connection not found !" << endl;
		exit(EXIT_FAILURE);
	} else {
		if (m_connections.size() == 0) {
			for (size_t i=0; i < _poly; ++i) {
				// Use a local buffer
				if (modify_buffers && m_buffers.at(i)->is_joined())
					m_buffers.at(i)->unjoin();
				m_buffers.at(i)->clear(); // Write silence
				//if (m_is_tied)
					//m_tied_port->buffer(i)->join(m_buffers.at(i));
			}
		} else if (modify_buffers && m_connections.size() == 1) {
			// Share a buffer
			for (size_t i=0; i < _poly; ++i) {
				m_buffers.at(i)->join((*m_connections.begin())->buffer(i));
				//if (m_is_tied)
				//	m_tied_port->buffer(i)->join(m_buffers.at(i));
			}
		}	
	}

	if (modify_buffers)
		TypedPort<T>::connect_buffers();

	//assert( ! m_is_tied || m_tied_port != NULL);
	//assert( ! m_is_tied || m_buffers.at(0)->data() == m_tied_port->buffer(0)->data());

	return connection;
}
template ListNode<TypedConnection<Sample>*>*
InputPort<Sample>::remove_connection(const OutputPort<Sample>* const src_port);
template ListNode<TypedConnection<MidiMessage>*>*
InputPort<MidiMessage>::remove_connection(const OutputPort<MidiMessage>* const src_port);


/** Returns whether this port is connected to the passed port.
 */
template <typename T>
bool
InputPort<T>::is_connected_to(const OutputPort<T>* const port) const
{
	typedef typename List<TypedConnection<T>*>::const_iterator TypedConnectionListIterator;
	for (TypedConnectionListIterator i = m_connections.begin(); i != m_connections.end(); ++i)
		if ((*i)->src_port() == port)
			return true;
	
	return false;
}
template bool InputPort<Sample>::is_connected_to(const OutputPort<Sample>* const port) const;
template bool InputPort<MidiMessage>::is_connected_to(const OutputPort<MidiMessage>* const port) const;


/** Prepare buffer for access, mixing if necessary.  Realtime safe.
 *  FIXME: nframes parameter not used,
 */
template<>
void
InputPort<Sample>::process(SampleCount nframes, FrameTime start, FrameTime end)
{
	//assert(!m_is_tied || m_tied_port != NULL);

	typedef List<TypedConnection<Sample>*>::iterator TypedConnectionListIterator;
	bool do_mixdown = true;
	
	if (m_connections.size() == 0) return;

	for (TypedConnectionListIterator c = m_connections.begin(); c != m_connections.end(); ++c)
		(*c)->process(nframes, start, end);

	// If only one connection, buffer is (maybe) used directly (no copying)
	if (m_connections.size() == 1) {
		// Buffer changed since connection
		if (m_buffers.at(0)->data() != (*m_connections.begin())->buffer(0)->data()) {
			if (m_fixed_buffers) { // || (m_is_tied && m_tied_port->fixed_buffers())) {
				// can't change buffer, must copy
				do_mixdown = true;
			} else {
				// zero-copy
				assert(m_buffers.at(0)->is_joined());
				m_buffers.at(0)->join((*m_connections.begin())->buffer(0));
				do_mixdown = false;
			}
			connect_buffers();
		} else {
			do_mixdown = false;
		}
	}

	//cerr << path() << " mixing: " << do_mixdown << endl;

	if (!do_mixdown) {
		assert(m_buffers.at(0)->data() == (*m_connections.begin())->buffer(0)->data());
		return;
	}

	/*assert(!m_is_tied || m_tied_port != NULL);
	assert(!m_is_tied || m_buffers.at(0)->data() == m_tied_port->buffer(0)->data());*/

	for (size_t voice=0; voice < _poly; ++voice) {
		// Copy first connection
		m_buffers.at(voice)->copy((*m_connections.begin())->buffer(voice), 0, _buffer_size-1);
		
		// Accumulate the rest
		if (m_connections.size() > 1) {

			TypedConnectionListIterator c = m_connections.begin();

			for (++c; c != m_connections.end(); ++c)
				m_buffers.at(voice)->accumulate((*c)->buffer(voice), 0, _buffer_size-1);
		}
	}
}


/** Prepare buffer for access, realtime safe.
 *
 * MIDI mixing not yet implemented.
 */
template <>
void
InputPort<MidiMessage>::process(SampleCount nframes, FrameTime start, FrameTime end)
{	
	//assert(!m_is_tied || m_tied_port != NULL);
	
	const size_t num_ins = m_connections.size();
	bool         do_mixdown = true;
	
	assert(num_ins == 0 || num_ins == 1);
	
	typedef List<TypedConnection<MidiMessage>*>::iterator TypedConnectionListIterator;
	assert(_poly == 1);
	
	for (TypedConnectionListIterator c = m_connections.begin(); c != m_connections.end(); ++c)
		(*c)->process(nframes, start, end);
	

	// If only one connection, buffer is used directly (no copying)
	if (num_ins == 1) {
		// Buffer changed since connection
		if (m_buffers.at(0) != (*m_connections.begin())->buffer(0)) {
			if (m_fixed_buffers) { // || (m_is_tied && m_tied_port->fixed_buffers())) {
				// can't change buffer, must copy
				do_mixdown = true;
			} else {
				// zero-copy
				m_buffers.at(0)->join((*m_connections.begin())->buffer(0));
				//if (m_is_tied)
				//	m_tied_port->buffer(0)->join(m_buffers.at(0));
				do_mixdown = false;
			}
			connect_buffers();
		} else {
			do_mixdown = false;
		}
		//assert(!m_is_tied || m_tied_port != NULL);
		//assert(!m_is_tied || m_buffers.at(0)->data() == m_tied_port->buffer(0)->data());
		//assert(!m_is_tied || m_buffers.at(0)->filled_size() == m_tied_port->buffer(0)->filled_size());
		assert(do_mixdown || m_buffers.at(0)->filled_size() ==
				(*m_connections.begin())->src_port()->buffer(0)->filled_size());
	}
	
	// Get valid buffer size from inbound connections, unless a port on a top-level
	// patch (which will be fed by the MidiDriver)
	if (_parent->parent() != NULL) {
		if (num_ins == 1) {
			m_buffers.at(0)->filled_size(
				(*m_connections.begin())->src_port()->buffer(0)->filled_size());
			
			//if (m_is_tied)
			//	m_tied_port->buffer(0)->filled_size(m_buffers.at(0)->filled_size());
			
			assert(m_buffers.at(0)->filled_size() ==
				(*m_connections.begin())->src_port()->buffer(0)->filled_size());
		} else {
			// Mixing not implemented
			m_buffers.at(0)->clear();
		}
	}

	//assert(!m_is_tied || m_buffers.at(0)->data() == m_tied_port->buffer(0)->data());

	if (!do_mixdown || m_buffers.at(0)->filled_size() == 0 || num_ins == 0)
		return;

	//cerr << path() << " - Copying MIDI buffer" << endl;
	
	// Be sure buffers are the same as tied port's, if joined
	//assert(!m_is_tied || m_tied_port != NULL);
	//assert(!m_is_tied || m_buffers.at(0)->data() == m_tied_port->buffer(0)->data());

	if (num_ins > 0)
		for (size_t i=0; i < m_buffers.at(0)->filled_size(); ++i)
			m_buffers.at(0)[i] = (*m_connections.begin())->buffer(0)[i];
}


template <typename T>
void
InputPort<T>::set_buffer_size(size_t size)
{
	TypedPort<T>::set_buffer_size(size);
	assert(_buffer_size = size);

	for (typename List<TypedConnection<T>*>::iterator c = m_connections.begin(); c != m_connections.end(); ++c)
		(*c)->set_buffer_size(size);
	
}
template void InputPort<Sample>::set_buffer_size(size_t size);
template void InputPort<MidiMessage>::set_buffer_size(size_t size);


} // namespace Ingen

