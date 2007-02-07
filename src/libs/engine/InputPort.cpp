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
	_connections.push_back(c);

	bool modify_buffers = !_fixed_buffers;
	//if (modify_buffers && _is_tied)
	//	modify_buffers = !_tied_port->fixed_buffers();
	
	if (modify_buffers) {
		if (_connections.size() == 1) {
			// Use buffer directly to avoid copying
			for (size_t i=0; i < _poly; ++i) {
				_buffers.at(i)->join(c->elem()->buffer(i));
				//if (_is_tied)
				//	_tied_port->buffer(i)->join(_buffers.at(i));
				assert(_buffers.at(i)->data() == c->elem()->buffer(i)->data());
			}
		} else if (_connections.size() == 2) {
			// Used to directly use single connection buffer, now there's two
			// so have to use local ones again and mix down
			for (size_t i=0; i < _poly; ++i) {
				_buffers.at(i)->unjoin();
				//if (_is_tied)
				//	_tied_port->buffer(i)->join(_buffers.at(i));
			}
		}
		TypedPort<T>::connect_buffers();
	}
		
	//assert( ! _is_tied || _tied_port != NULL);
	//assert( ! _is_tied || _buffers.at(0)->data() == _tied_port->buffer(0)->data());
}
template void InputPort<Sample>::add_connection(ListNode<TypedConnection<Sample>*>* const c);
template void InputPort<MidiMessage>::add_connection(ListNode<TypedConnection<MidiMessage>*>* const c);


/** Remove a connection.  Realtime safe.
 */
template <typename T>
ListNode<TypedConnection<T>*>*
InputPort<T>::remove_connection(const OutputPort<T>* const src_port)
{
	bool modify_buffers = !_fixed_buffers;
	//if (modify_buffers && _is_tied)
	//	modify_buffers = !_tied_port->fixed_buffers();
	
	typedef typename List<TypedConnection<T>*>::iterator TypedConnectionListIterator;
	bool found = false;
	ListNode<TypedConnection<T>*>* connection = NULL;
	for (TypedConnectionListIterator i = _connections.begin(); i != _connections.end(); ++i) {
		if ((*i)->src_port()->path() == src_port->path()) {
			connection = _connections.remove(i);
			found = true;
		}
	}

	if ( ! found) {
		cerr << "WARNING:  [InputPort<T>::remove_connection] Connection not found !" << endl;
		exit(EXIT_FAILURE);
	} else {
		if (_connections.size() == 0) {
			for (size_t i=0; i < _poly; ++i) {
				// Use a local buffer
				if (modify_buffers && _buffers.at(i)->is_joined())
					_buffers.at(i)->unjoin();
				_buffers.at(i)->clear(); // Write silence
				//if (_is_tied)
					//m_tied_port->buffer(i)->join(_buffers.at(i));
			}
		} else if (modify_buffers && _connections.size() == 1) {
			// Share a buffer
			for (size_t i=0; i < _poly; ++i) {
				_buffers.at(i)->join((*_connections.begin())->buffer(i));
				//if (_is_tied)
				//	_tied_port->buffer(i)->join(_buffers.at(i));
			}
		}	
	}

	if (modify_buffers)
		TypedPort<T>::connect_buffers();

	//assert( ! _is_tied || _tied_port != NULL);
	//assert( ! _is_tied || _buffers.at(0)->data() == _tied_port->buffer(0)->data());

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
	for (TypedConnectionListIterator i = _connections.begin(); i != _connections.end(); ++i)
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
	//assert(!_is_tied || _tied_port != NULL);

	typedef List<TypedConnection<Sample>*>::iterator TypedConnectionListIterator;
	bool do_mixdown = true;
	
	if (_connections.size() == 0) return;

	for (TypedConnectionListIterator c = _connections.begin(); c != _connections.end(); ++c)
		(*c)->process(nframes, start, end);

	// If only one connection, buffer is (maybe) used directly (no copying)
	if (_connections.size() == 1) {
		// Buffer changed since connection
		if (_buffers.at(0)->data() != (*_connections.begin())->buffer(0)->data()) {
			if (_fixed_buffers) { // || (_is_tied && _tied_port->fixed_buffers())) {
				// can't change buffer, must copy
				do_mixdown = true;
			} else {
				// zero-copy
				assert(_buffers.at(0)->is_joined());
				_buffers.at(0)->join((*_connections.begin())->buffer(0));
				do_mixdown = false;
			}
			connect_buffers();
		} else {
			do_mixdown = false;
		}
	}

	//cerr << path() << " mixing: " << do_mixdown << endl;

	if (!do_mixdown) {
		assert(_buffers.at(0)->data() == (*_connections.begin())->buffer(0)->data());
		return;
	}

	/*assert(!_is_tied || _tied_port != NULL);
	assert(!_is_tied || _buffers.at(0)->data() == _tied_port->buffer(0)->data());*/

	for (size_t voice=0; voice < _poly; ++voice) {
		// Copy first connection
		_buffers.at(voice)->copy((*_connections.begin())->buffer(voice), 0, _buffer_size-1);
		
		// Accumulate the rest
		if (_connections.size() > 1) {

			TypedConnectionListIterator c = _connections.begin();

			for (++c; c != _connections.end(); ++c)
				_buffers.at(voice)->accumulate((*c)->buffer(voice), 0, _buffer_size-1);
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
	//assert(!_is_tied || _tied_port != NULL);
	
	const size_t num_ins = _connections.size();
	bool         do_mixdown = true;
	
	assert(num_ins == 0 || num_ins == 1);
	
	typedef List<TypedConnection<MidiMessage>*>::iterator TypedConnectionListIterator;
	assert(_poly == 1);
	
	for (TypedConnectionListIterator c = _connections.begin(); c != _connections.end(); ++c)
		(*c)->process(nframes, start, end);
	

	// If only one connection, buffer is used directly (no copying)
	if (num_ins == 1) {
		// Buffer changed since connection
		if (_buffers.at(0) != (*_connections.begin())->buffer(0)) {
			if (_fixed_buffers) { // || (_is_tied && _tied_port->fixed_buffers())) {
				// can't change buffer, must copy
				do_mixdown = true;
			} else {
				// zero-copy
				_buffers.at(0)->join((*_connections.begin())->buffer(0));
				//if (_is_tied)
				//	_tied_port->buffer(0)->join(_buffers.at(0));
				do_mixdown = false;
			}
			connect_buffers();
		} else {
			do_mixdown = false;
		}
		//assert(!_is_tied || _tied_port != NULL);
		//assert(!_is_tied || _buffers.at(0)->data() == _tied_port->buffer(0)->data());
		//assert(!_is_tied || _buffers.at(0)->filled_size() == _tied_port->buffer(0)->filled_size());
		assert(do_mixdown || _buffers.at(0)->filled_size() ==
				(*_connections.begin())->src_port()->buffer(0)->filled_size());
	}
	
	// Get valid buffer size from inbound connections, unless a port on a top-level
	// patch (which will be fed by the MidiDriver)
	if (_parent->parent() != NULL) {
		if (num_ins == 1) {
			_buffers.at(0)->filled_size(
				(*_connections.begin())->src_port()->buffer(0)->filled_size());
			
			//if (_is_tied)
			//	_tied_port->buffer(0)->filled_size(_buffers.at(0)->filled_size());
			
			assert(_buffers.at(0)->filled_size() ==
				(*_connections.begin())->src_port()->buffer(0)->filled_size());
		} else {
			// Mixing not implemented
			_buffers.at(0)->clear();
		}
	}

	//assert(!_is_tied || _buffers.at(0)->data() == _tied_port->buffer(0)->data());

	if (!do_mixdown || _buffers.at(0)->filled_size() == 0 || num_ins == 0)
		return;

	//cerr << path() << " - Copying MIDI buffer" << endl;
	
	// Be sure buffers are the same as tied port's, if joined
	//assert(!_is_tied || _tied_port != NULL);
	//assert(!_is_tied || _buffers.at(0)->data() == _tied_port->buffer(0)->data());

	if (num_ins > 0)
		for (size_t i=0; i < _buffers.at(0)->filled_size(); ++i)
			_buffers.at(0)[i] = (*_connections.begin())->buffer(0)[i];
}


template <typename T>
void
InputPort<T>::set_buffer_size(size_t size)
{
	TypedPort<T>::set_buffer_size(size);
	assert(_buffer_size = size);

	for (typename List<TypedConnection<T>*>::iterator c = _connections.begin(); c != _connections.end(); ++c)
		(*c)->set_buffer_size(size);
	
}
template void InputPort<Sample>::set_buffer_size(size_t size);
template void InputPort<MidiMessage>::set_buffer_size(size_t size);


} // namespace Ingen

