/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
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

#include <cstdlib>
#include <cassert>

#include "ingen/Patch.hpp"

#include "AudioBuffer.hpp"
#include "BufferFactory.hpp"
#include "ConnectionImpl.hpp"
#include "EventBuffer.hpp"
#include "InputPort.hpp"
#include "NodeImpl.hpp"
#include "Notification.hpp"
#include "OutputPort.hpp"
#include "ProcessContext.hpp"
#include "ThreadManager.hpp"
#include "mix.hpp"
#include "ingen/shared/LV2URIMap.hpp"
#include "util.hpp"

using namespace std;

namespace Ingen {
namespace Server {

InputPort::InputPort(BufferFactory&      bufs,
                     NodeImpl*           parent,
                     const Raul::Symbol& symbol,
                     uint32_t            index,
                     uint32_t            poly,
                     PortType            type,
                     const Raul::Atom&   value,
                     size_t              buffer_size)
	: PortImpl(bufs, parent, symbol, index, poly, type, value, buffer_size)
	, _num_connections(0)
{
	const Ingen::Shared::URIs& uris = bufs.uris();

	if (!dynamic_cast<Patch*>(parent))
		add_property(uris.rdf_type, uris.lv2_InputPort);

	// Set default control range
	if (type == PortType::CONTROL) {
		set_property(uris.lv2_minimum, 0.0f);
		set_property(uris.lv2_maximum, 1.0f);
	}
}

bool
InputPort::apply_poly(Maid& maid, uint32_t poly)
{
	bool ret = PortImpl::apply_poly(maid, poly);
	if (!ret)
		poly = 1;

	assert(_buffers->size() >= poly);

	return true;
}

/** Set @a buffers to the buffers to be used for this port.
 * @return true iff buffers are locally owned by the port
 */
bool
InputPort::get_buffers(BufferFactory&                   bufs,
                       Raul::Array<BufferFactory::Ref>* buffers,
                       uint32_t                         poly) const
{
	size_t num_connections = (ThreadManager::thread_is(THREAD_PROCESS))
			? _connections.size() : _num_connections;

	if (is_a(PortType::AUDIO) && num_connections == 0) {
		// Audio input with no connections, use shared zero buffer
		for (uint32_t v = 0; v < poly; ++v)
			buffers->at(v) = bufs.silent_buffer();
		return false;

	} else if (num_connections == 1) {
		if (ThreadManager::thread_is(THREAD_PROCESS)) {
			if (!_connections.front().must_mix() && !_connections.front().must_queue()) {
				// Single non-mixing conneciton, use buffers directly
				for (uint32_t v = 0; v < poly; ++v)
					buffers->at(v) = _connections.front().buffer(v);
				return false;
			}
		}
	}

	// Otherwise, allocate local buffers
	for (uint32_t v = 0; v < poly; ++v) {
		buffers->at(v) = _bufs.get(buffer_type(), _buffer_size);
		buffers->at(v)->clear();
	}
	return true;
}

/** Add a connection.  Realtime safe.
 *
 * The buffer of this port will be set directly to the connection's buffer
 * if there is only one connection, since no copying/mixing needs to take place.
 *
 * Note that setup_buffers must be called after this before the change
 * will audibly take effect.
 */
void
InputPort::add_connection(ConnectionImpl* c)
{
	ThreadManager::assert_thread(THREAD_PROCESS);

	_connections.push_front(*c);

	// Broadcast value/activity of connected input
	_broadcast = true;
}

/** Remove a connection.  Realtime safe.
 *
 * Note that setup_buffers must be called after this before the change
 * will audibly take effect.
 */
ConnectionImpl*
InputPort::remove_connection(ProcessContext& context, const OutputPort* src_port)
{
	ThreadManager::assert_thread(THREAD_PROCESS);

	ConnectionImpl* connection = NULL;
	for (Connections::iterator i = _connections.begin(); i != _connections.end(); ++i) {
		if (i->src_port() == src_port) {
			connection = &*i;
			_connections.erase(i);
			break;
		}
	}

	if (!connection) {
		error << "[InputPort::remove_connection] Connection not found!" << endl;
		return NULL;
	}

	// Turn off broadcasting if we're no longer connected
	if (_connections.empty()) {
		if (is_a(PortType::AUDIO)) {
			// Send an update peak of 0.0 to reset to silence
			const Notification note = Notification::make(
				Notification::PORT_ACTIVITY, context.start(), this, 0.0f);
			context.event_sink().write(sizeof(note), &note);
		}
		_broadcast = false;
	}

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

	if (_connections.empty()) {
		for (uint32_t v = 0; v < _poly; ++v) {
			buffer(v)->prepare_read(context);
		}
	} else if (direct_connect()) {
		for (uint32_t v = 0; v < _poly; ++v) {
			_buffers->at(v) = _connections.front().buffer(v);
			_buffers->at(v)->prepare_read(context);
		}
	} else {
		uint32_t max_num_srcs = 0;
		for (Connections::const_iterator c = _connections.begin();
		     c != _connections.end(); ++c) {
			max_num_srcs += c->src_port()->poly();
		}

		boost::intrusive_ptr<Buffer> srcs[max_num_srcs];

		for (uint32_t v = 0; v < _poly; ++v) {
			uint32_t num_srcs = 0;
			for (Connections::iterator c = _connections.begin();
			     c != _connections.end(); ++c) {
				c->get_sources(context, v, srcs, max_num_srcs, num_srcs);
			}

			mix(context, buffer(v).get(), srcs, num_srcs);
			buffer(v)->prepare_read(context);
		}
	}

	if (_broadcast)
		broadcast_value(context, false);
}

void
InputPort::post_process(Context& context)
{
	if (_set_by_user) {
		if (is_a(PortType::EVENTS)) {
			// Clear events received via a SetPortValue
			for (uint32_t v = 0; v < _poly; ++v) {
				buffer(v)->clear();
			}
		}
		_set_by_user = false;
	}
}

bool
InputPort::direct_connect() const
{
	return (context() == Context::AUDIO)
		&& _connections.size() == 1
		&& !_connections.front().must_mix()
		&& !_connections.front().must_queue();
}

} // namespace Server
} // namespace Ingen

