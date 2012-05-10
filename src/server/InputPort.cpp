/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cstdlib>
#include <cassert>

#include "ingen/Patch.hpp"

#include "AudioBuffer.hpp"
#include "BufferFactory.hpp"
#include "ConnectionImpl.hpp"
#include "Engine.hpp"
#include "InputPort.hpp"
#include "NodeImpl.hpp"
#include "Notification.hpp"
#include "OutputPort.hpp"
#include "ProcessContext.hpp"
#include "ThreadManager.hpp"
#include "ingen/shared/URIs.hpp"
#include "mix.hpp"
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
                     LV2_URID            buffer_type,
                     const Raul::Atom&   value,
                     size_t              buffer_size)
	: PortImpl(bufs, parent, symbol, index, poly, type, buffer_type, value, buffer_size)
	, _num_connections(0)
{
	const Ingen::Shared::URIs& uris = bufs.uris();

	if (!dynamic_cast<Patch*>(parent))
		add_property(uris.rdf_type, uris.lv2_InputPort);

	// Set default control range
	if (type == PortType::CONTROL || type == PortType::CV) {
		set_property(uris.lv2_minimum, bufs.forge().make(0.0f));
		set_property(uris.lv2_maximum, bufs.forge().make(1.0f));
	}
}

bool
InputPort::apply_poly(Raul::Maid& maid, uint32_t poly)
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
InputPort::get_buffers(BufferFactory&          bufs,
                       Raul::Array<BufferRef>* buffers,
                       uint32_t                poly) const
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
			if (!_connections.front().must_mix() &&
			    !_connections.front().must_queue()) {
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
InputPort::remove_connection(ProcessContext& context, const OutputPort* tail)
{
	ThreadManager::assert_thread(THREAD_PROCESS);

	ConnectionImpl* connection = NULL;
	for (Connections::iterator i = _connections.begin(); i != _connections.end(); ++i) {
		if (i->tail() == tail) {
			connection = &*i;
			_connections.erase(i);
			break;
		}
	}

	if (!connection) {
		Raul::error << "[InputPort::remove_connection] Connection not found!"
		            << std::endl;
		return NULL;
	}

	// Turn off broadcasting if we're no longer connected
	if (_connections.empty()) {
		if (is_a(PortType::AUDIO)) {
			// Send an update peak of 0.0 to reset to silence
			const Notification note = Notification::make(
				Notification::PORT_ACTIVITY, context.start(), this,
				context.engine().world()->forge().make(0.0f));
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
			max_num_srcs += c->tail()->poly();
		}

		boost::intrusive_ptr<Buffer> srcs[max_num_srcs];

		for (uint32_t v = 0; v < _poly; ++v) {
			uint32_t num_srcs = 0;
			for (Connections::iterator c = _connections.begin();
			     c != _connections.end(); ++c) {
				c->get_sources(context, v, srcs, max_num_srcs, num_srcs);
			}

			mix(context, bufs().uris(), buffer(v).get(), srcs, num_srcs);
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
		if (_buffer_type == _bufs.uris().atom_Sequence) {
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
	return _connections.size() == 1
		&& !_connections.front().must_mix()
		&& !_connections.front().must_queue();
}

} // namespace Server
} // namespace Ingen

