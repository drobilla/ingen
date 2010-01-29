/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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
#include <cstdlib>
#include <cassert>
#include "interface/Patch.hpp"
#include "AudioBuffer.hpp"
#include "BufferFactory.hpp"
#include "ConnectionImpl.hpp"
#include "EventBuffer.hpp"
#include "NodeImpl.hpp"
#include "OutputPort.hpp"
#include "ProcessContext.hpp"
#include "ThreadManager.hpp"
#include "mix.hpp"
#include "util.hpp"

using namespace std;

namespace Ingen {

namespace Shared { class Patch; }
using namespace Shared;

InputPort::InputPort(BufferFactory&    bufs,
                     NodeImpl*         parent,
                     const string&     name,
                     uint32_t          index,
                     uint32_t          poly,
                     PortType          type,
                     const Raul::Atom& value,
                     size_t            buffer_size)
	: PortImpl(bufs, parent, name, index, poly, type, value, buffer_size)
{
	if (!dynamic_cast<Patch*>(parent))
		add_property("rdf:type", Raul::Atom(Raul::Atom::URI, "lv2:InputPort"));

	// Set default control range
	if (type == PortType::CONTROL) {
		set_property("lv2:minimum", 0.0f);
		set_property("lv2:maximum", 1.0f);
	}
}


bool
InputPort::can_direct() const
{
	return _connections.size() == 1
		&& _connections.front()->src_port()->poly() == poly()
		&& (_connections.front()->src_port()->type() == type()
			|| (_connections.front()->src_port()->type() == PortType::AUDIO
				&& type() == PortType::CONTROL));
}


void
InputPort::set_buffer_size(BufferFactory& bufs, size_t size)
{
	PortImpl::set_buffer_size(bufs, size);
	assert(_buffer_size = size);

	for (Connections::iterator c = _connections.begin(); c != _connections.end(); ++c)
		((ConnectionImpl*)c->get())->set_buffer_size(bufs, size);
}


bool
InputPort::prepare_poly(BufferFactory& bufs, uint32_t poly)
{
	PortImpl::prepare_poly(bufs, poly);

	for (Connections::iterator c = _connections.begin(); c != _connections.end(); ++c)
		((ConnectionImpl*)c->get())->prepare_poly(bufs, poly);

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

	if (_connections.size() == 1) {
		ConnectionImpl* c = _connections.begin()->get();
		for (uint32_t v = _poly; v < poly; ++v)
			_buffers->at(v) = c->buffer(v);
	}

	for (uint32_t i = 0; i < _poly; ++i)
		PortImpl::parent_node()->set_port_buffer(i, _index, buffer(i));

	return true;
}


/** Connect buffers to the appropriate locations based on the current connections */
void
InputPort::connect_buffers()
{
	// Single connection now, use it directly
	if (_connections.size() == 1) {
		for (uint32_t v = 0; v < _poly; ++v)
			_buffers->at(v) = _connections.front()->buffer(v);

	// Use local buffers
	} else {
		for (uint32_t v = 0; v < _poly; ++v)
			_buffers->at(v) = _bufs.get(_type, _buffer_size); // Use local buffer
	}

	// Connect node to buffers
	PortImpl::connect_buffers();
}


/** Add a connection.  Realtime safe.
 *
 * The buffer of this port will be set directly to the connection's buffer
 * if there is only one connection, since no copying/mixing needs to take place.
 */
void
InputPort::add_connection(Connections::Node* const c)
{
	ThreadManager::assert_thread(THREAD_PROCESS);

	_connections.push_back(c);
	connect_buffers();

	// Automatically broadcast connected control inputs
	if (_type == PortType::CONTROL)
		_broadcast = true;
}


/** Remove a connection.  Realtime safe. */
InputPort::Connections::Node*
InputPort::remove_connection(const OutputPort* src_port)
{
	ThreadManager::assert_thread(THREAD_PROCESS);

	Connections::Node* connection = NULL;
	for (Connections::iterator i = _connections.begin(); i != _connections.end(); ++i)
		if ((*i)->src_port() == src_port)
			connection = _connections.erase(i);

	if ( ! connection) {
		error << "[InputPort::remove_connection] Connection not found!" << endl;
		return NULL;
	}

	connect_buffers();

	if (_connections.size() == 0)
		for (uint32_t v = 0; v < _poly; ++v)
			buffer(v)->clear();

	// Turn off broadcasting if we're no longer connected
	if (_type == PortType::CONTROL && _connections.size() == 0)
		_broadcast = false;

	return connection;
}


/** Prepare buffer for access, mixing if necessary.  Realtime safe.
 */
void
InputPort::pre_process(Context& context)
{
	// If value has been set (e.g. events pushed) by the user, don't smash it
	if (_set_by_user)
		return;

	//connect_buffers();

	// Process connections (mix down polyphony, if necessary)
	for (Connections::iterator c = _connections.begin(); c != _connections.end(); ++c)
		(*c)->process(context);

	// Multiple connections, mix them all into our local buffers
	if (_connections.size() > 1) {
		for (uint32_t v = 0; v < _poly; ++v) {
			const uint32_t        num_srcs = _connections.size();
			Connections::iterator c        = _connections.begin();
			Buffer* srcs[num_srcs];
			for (uint32_t i = 0; c != _connections.end(); ++c) {
				assert(i < num_srcs);
				srcs[i++] = (*c)->buffer(v).get();
			}

			mix(context, buffer(v).get(), srcs, num_srcs);
		}
	}

	for (uint32_t v = 0; v < _poly; ++v)
		buffer(v)->prepare_read(context);

	if (_broadcast)
		broadcast_value(context, false);
}


void
InputPort::post_process(Context& context)
{
	// Prepare buffers for next cycle
	if (!can_direct())
		for (uint32_t v = 0; v < _poly; ++v)
			buffer(v)->prepare_write(context);

	_set_by_user = false;
}


} // namespace Ingen

