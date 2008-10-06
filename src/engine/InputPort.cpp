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
#include "EventBuffer.hpp"
#include "ConnectionImpl.hpp"
#include "OutputPort.hpp"
#include "NodeImpl.hpp"
#include "ProcessContext.hpp"
#include "util.hpp"

using namespace std;

namespace Ingen {


InputPort::InputPort(NodeImpl*     parent,
                     const string& name,
                     uint32_t      index,
                     uint32_t      poly,
                     DataType      type,
                     const Atom&   value,
                     size_t        buffer_size)
	: PortImpl(parent, name, index, poly, type, value, buffer_size)
{
}
	

bool
InputPort::can_direct() const
{
	return _connections.size() == 1 && _connections.front()->can_direct();
}


void
InputPort::set_buffer_size(size_t size)
{
	PortImpl::set_buffer_size(size);
	assert(_buffer_size = size);

	for (Connections::iterator c = _connections.begin(); c != _connections.end(); ++c)
		((ConnectionImpl*)c->get())->set_buffer_size(size);
	
}

	
bool
InputPort::prepare_poly(uint32_t poly)
{
	PortImpl::prepare_poly(poly);

	for (Connections::iterator c = _connections.begin(); c != _connections.end(); ++c)
		((ConnectionImpl*)c->get())->prepare_poly(poly);

	return true;
}


bool
InputPort::apply_poly(Raul::Maid& maid, uint32_t poly)
{
	if (!_polyphonic || !_parent->polyphonic())
		return true;
	
	for (Connections::iterator c = _connections.begin(); c != _connections.end(); ++c)
		((ConnectionImpl*)c->get())->apply_poly(maid, poly);

	PortImpl::apply_poly(maid, poly);
	assert(this->poly() == poly);
		
	if (can_direct()) {
		ConnectionImpl* c = _connections.begin()->get();
		for (uint32_t i=_poly; i < poly; ++i)
			_buffers->at(i)->join(c->buffer(i));
	}
		
	for (uint32_t i=0; i < _poly; ++i)
		PortImpl::parent_node()->set_port_buffer(i, _index, buffer(i));

	return true;
}


/** Add a connection.  Realtime safe.
 *
 * The buffer of this port will be set directly to the connection's buffer
 * if there is only one connection, since no mixing needs to take place.
 */
void
InputPort::add_connection(Connections::Node* const c)
{
	_connections.push_back(c);

	bool modify_buffers = !_fixed_buffers;
	
	if (modify_buffers) {
		if (can_direct()) {
			// Use buffer directly to avoid copying
			for (uint32_t i=0; i < _poly; ++i) {
				_buffers->at(i)->join(c->elem()->buffer(i));
			}
		} else if (_connections.size() == 2) {
			// Used to directly use single connection buffer, now there's two
			// so have to use local ones again and mix down
			for (uint32_t i=0; i < _poly; ++i) {
				_buffers->at(i)->unjoin();
			}
		}
		PortImpl::connect_buffers();
	}

	// Automatically broadcast connected control inputs
	if (_type == DataType::CONTROL)
		_broadcast = true;
}


/** Remove a connection.  Realtime safe.
 */
InputPort::Connections::Node*
InputPort::remove_connection(const OutputPort* src_port)
{
	bool modify_buffers = !_fixed_buffers;
	
	bool found = false;
	Connections::Node* connection = NULL;
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
					_buffers->at(i)->unjoin();
				_buffers->at(i)->clear(); // Write silence
			}
		} else if (modify_buffers && _connections.size() == 1) {
			// Share a buffer
			for (uint32_t i=0; i < _poly; ++i) {
				_buffers->at(i)->join(_connections.front()->buffer(i));
			}
		}	
	}

	if (modify_buffers)
		PortImpl::connect_buffers();
	
	// Turn off broadcasting if we're not connected any more (FIXME: not quite right..)
	if (_type == DataType::CONTROL && _connections.size() == 0)
		_broadcast = false;

	return connection;
}


/** Prepare buffer for access, mixing if necessary.  Realtime safe.
 *  FIXME: nframes parameter not used,
 */
void
InputPort::pre_process(ProcessContext& context)
{
	// If value has been set (e.g. events pushed) by the user,
	// don't do anything this cycle to avoid smashing the value
	if (_set_by_user)
		return;

	bool do_mixdown = true;
	
	if (_connections.size() == 0) {
		for (uint32_t i=0; i < _poly; ++i)
			buffer(i)->prepare_read(context.start(), context.nframes());
		return;
	}

	for (Connections::iterator c = _connections.begin(); c != _connections.end(); ++c)
		(*c)->process(context);

	if ( ! _fixed_buffers) {
		// If only one connection, try to use buffer directly (zero copy)
		if (can_direct()) {
			for (uint32_t i=0; i < _poly; ++i) {
				_buffers->at(i)->join(_connections.front()->buffer(i));
			}
			do_mixdown = false;
		}
		connect_buffers();
	} else {
		do_mixdown = true;
	}

	for (uint32_t i=0; i < _poly; ++i)
		buffer(i)->prepare_read(context.start(), context.nframes());
	
	/*cerr << path() << " poly = " << _poly << ", mixdown: " << do_mixdown
		<< ", fixed buffers: " << _fixed_buffers << ", joined: " << _buffers->at(0)->is_joined()
		<< " to " << _buffers->at(0)->joined_buffer() << endl;*/
	
	/*if (type() == DataType::EVENT) 
		for (uint32_t i=0; i < _poly; ++i)
			cerr << path() << " (" << buffer(i) << ") # events: "
				<< ((EventBuffer*)buffer(i))->event_count()
				<< ", joined: " << _buffers->at(i)->is_joined() << endl;*/

	if (!do_mixdown) {
		/*#ifndef NDEBUG
		for (uint32_t i=0; i < _poly; ++i)
			assert(buffer(i) == _connections.front()->buffer(i));
		#endif*/
		return;
	}

	if (_type == DataType::CONTROL || _type == DataType::AUDIO) {
		for (uint32_t voice=0; voice < _poly; ++voice) {
			// Copy first connection
			buffer(voice)->copy(
				_connections.front()->buffer(voice), 0, _buffer_size-1);

			// Accumulate the rest
			if (_connections.size() > 1) {

				Connections::iterator c = _connections.begin();

				for (++c; c != _connections.end(); ++c)
					((AudioBuffer*)buffer(voice))->accumulate(
					((AudioBuffer*)(*c)->buffer(voice)), 0, _buffer_size-1);
			}
		}
	} else {
		assert(_poly == 1);
		
		// FIXME
		if (_connections.size() > 1)
			cerr << "WARNING: MIDI mixing not implemented, only first connection used." << endl;
			
		// Copy first connection
		_buffers->at(0)->copy(_connections.front()->buffer(0), 0, _buffer_size-1);
	}
}


void
InputPort::post_process(ProcessContext& context)
{
	broadcast(context);

	// Prepare for next cycle
	for (uint32_t i=0; i < _poly; ++i)
		buffer(i)->prepare_write(context.start(), context.nframes());

	_set_by_user = false;
	
	/*if (_broadcast && (_type == DataType::CONTROL)) {
		const Sample value = ((AudioBuffer*)(*_buffers)[0])->value_at(0);

		cerr << path() << " input post: buffer: " << buffer(0) << " value = "
			<< value << " (last " << _last_broadcasted_value << ")" <<endl;
	}*/
}


} // namespace Ingen

