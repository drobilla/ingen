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

#include "InputPort.hpp"
#include <iostream>
#include <cstdlib>
#include <cassert>
#include "AudioBuffer.hpp"
#include "Connection.hpp"
#include "OutputPort.hpp"
#include "Node.hpp"
#include "util.hpp"

using std::cerr; using std::cout; using std::endl;


namespace Ingen {


InputPort::InputPort(Node* parent, const string& name, uint32_t index, uint32_t poly, DataType type, size_t buffer_size)
: Port(parent, name, index, poly, type, buffer_size)
{
}


/** Add a connection.  Realtime safe.
 *
 * The buffer of this port will be set directly to the connection's buffer
 * if there is only one connection, since no mixing needs to take place.
 */
void
InputPort::add_connection(Raul::ListNode<Connection*>* const c)
{
	_connections.push_back(c);

	bool modify_buffers = !_fixed_buffers;
	//if (modify_buffers && _is_tied)
	//	modify_buffers = !_tied_port->fixed_buffers();
	
	if (modify_buffers) {
		if (_connections.size() == 1) {
			// Use buffer directly to avoid copying
			for (uint32_t i=0; i < _poly; ++i) {
				_buffers.at(i)->join(c->elem()->buffer(i));
				//if (_is_tied)
				//	_tied_port->buffer(i)->join(_buffers.at(i));
				//assert(_buffers.at(i)->data() == c->elem()->buffer(i)->data());
			}
		} else if (_connections.size() == 2) {
			// Used to directly use single connection buffer, now there's two
			// so have to use local ones again and mix down
			for (uint32_t i=0; i < _poly; ++i) {
				_buffers.at(i)->unjoin();
				//if (_is_tied)
				//	_tied_port->buffer(i)->join(_buffers.at(i));
			}
		}
		Port::connect_buffers();
	}
		
	//assert( ! _is_tied || _tied_port != NULL);
	//assert( ! _is_tied || _buffers.at(0)->data() == _tied_port->buffer(0)->data());
}


/** Remove a connection.  Realtime safe.
 */
Raul::ListNode<Connection*>*
InputPort::remove_connection(const OutputPort* src_port)
{
	bool modify_buffers = !_fixed_buffers;
	//if (modify_buffers && _is_tied)
	//	modify_buffers = !_tied_port->fixed_buffers();
	
	bool found = false;
	Raul::ListNode<Connection*>* connection = NULL;
	for (Connections::iterator i = _connections.begin(); i != _connections.end(); ++i) {
		if ((*i)->src_port()->path() == src_port->path()) {
			connection = _connections.erase(i);
			found = true;
		}
	}

	if ( ! found) {
		cerr << "WARNING:  [InputPort::remove_connection] Connection not found !" << endl;
		exit(EXIT_FAILURE);
	} else {
		if (_connections.size() == 0) {
			for (uint32_t i=0; i < _poly; ++i) {
				// Use a local buffer
				if (modify_buffers)
					_buffers.at(i)->unjoin();
				_buffers.at(i)->clear(); // Write silence
				//if (_is_tied)
					//m_tied_port->buffer(i)->join(_buffers.at(i));
			}
		} else if (modify_buffers && _connections.size() == 1) {
			// Share a buffer
			for (uint32_t i=0; i < _poly; ++i) {
				_buffers.at(i)->join((*_connections.begin())->buffer(i));
				//if (_is_tied)
				//	_tied_port->buffer(i)->join(_buffers.at(i));
			}
		}	
	}

	if (modify_buffers)
		Port::connect_buffers();

	//assert( ! _is_tied || _tied_port != NULL);
	//assert( ! _is_tied || _buffers.at(0)->data() == _tied_port->buffer(0)->data());

	return connection;
}


/** Returns whether this port is connected to the passed port.
 */
bool
InputPort::is_connected_to(const OutputPort* port) const
{
	for (Connections::const_iterator i = _connections.begin(); i != _connections.end(); ++i)
		if ((*i)->src_port() == port)
			return true;
	
	return false;
}


/** Prepare buffer for access, mixing if necessary.  Realtime safe.
 *  FIXME: nframes parameter not used,
 */
void
InputPort::pre_process(SampleCount nframes, FrameTime start, FrameTime end)
{
	//assert(!_is_tied || _tied_port != NULL);

	bool do_mixdown = true;
	
	if (_connections.size() == 0) {
		for (uint32_t i=0; i < _poly; ++i)
			_buffers.at(i)->prepare_read(nframes);
		return;
	}

	for (Connections::iterator c = _connections.begin(); c != _connections.end(); ++c)
		(*c)->process(nframes, start, end);

	// If only one connection, buffer is (maybe) used directly (no copying)
	if (_connections.size() == 1) {
		// Buffer changed since connection
		if (!_buffers.at(0)->is_joined_to((*_connections.begin())->buffer(0))) {
			if (_fixed_buffers) { // || (_is_tied && _tied_port->fixed_buffers())) {
				// can't change buffer, must copy
				do_mixdown = true;
			} else {
				// zero-copy
				_buffers.at(0)->join((*_connections.begin())->buffer(0));
				do_mixdown = false;
			}
			connect_buffers();
		} else {
			do_mixdown = false;
		}
	}

	//cerr << path() << " mixing: " << do_mixdown << endl;

	for (uint32_t i=0; i < _poly; ++i)
		_buffers.at(i)->prepare_read(nframes);
	
	if (_type == DataType::MIDI)
		cerr << path() << " nevents: " << ((MidiBuffer*)_buffers.at(0))->event_count() << endl;

	if (!do_mixdown) {
		assert(_buffers.at(0)->is_joined_to((*_connections.begin())->buffer(0)));
		return;
	}

	/*assert(!_is_tied || _tied_port != NULL);
	assert(!_is_tied || _buffers.at(0)->data() == _tied_port->buffer(0)->data());*/

	if (_type == DataType::FLOAT) {
		for (uint32_t voice=0; voice < _poly; ++voice) {
			// Copy first connection
			_buffers.at(voice)->copy(
				(*_connections.begin())->buffer(voice), 0, _buffer_size-1);

			// Accumulate the rest
			if (_connections.size() > 1) {

				Connections::iterator c = _connections.begin();

				for (++c; c != _connections.end(); ++c)
					((AudioBuffer*)_buffers.at(voice))->accumulate(
					((AudioBuffer*)(*c)->buffer(voice)), 0, _buffer_size-1);
			}
		}
	} else {
		assert(_poly == 1);
		
		// FIXME
		//if (_connections.size() > 1)
		//	cerr << "WARNING: MIDI mixing not implemented, only first connection used." << endl;
			
		// Copy first connection
		_buffers.at(0)->copy(
			(*_connections.begin())->buffer(0), 0, _buffer_size-1);
	}
}


void
InputPort::set_buffer_size(size_t size)
{
	Port::set_buffer_size(size);
	assert(_buffer_size = size);

	for (Raul::List<Connection*>::iterator c = _connections.begin(); c != _connections.end(); ++c)
		(*c)->set_buffer_size(size);
	
}

void
InputPort::post_process(SampleCount nframes, FrameTime start, FrameTime end)
{
	// Prepare for next cycle
	for (uint32_t i=0; i < _poly; ++i)
		_buffers.at(i)->prepare_write(nframes);
}


} // namespace Ingen

