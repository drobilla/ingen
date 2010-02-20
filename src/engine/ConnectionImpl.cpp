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

#include <algorithm>
#include "raul/log.hpp"
#include "raul/Maid.hpp"
#include "raul/IntrusivePtr.hpp"
#include "AudioBuffer.hpp"
#include "BufferFactory.hpp"
#include "ConnectionImpl.hpp"
#include "Engine.hpp"
#include "EventBuffer.hpp"
#include "InputPort.hpp"
#include "MessageContext.hpp"
#include "PortImpl.hpp"
#include "ProcessContext.hpp"
#include "mix.hpp"
#include "util.hpp"

namespace Ingen {

using namespace Shared;

/** Constructor for a connection from a node's output port.
 *
 * This handles both polyphonic and monophonic nodes, transparently to the
 * user (InputPort).
 */
ConnectionImpl::ConnectionImpl(BufferFactory& bufs, PortImpl* src_port, PortImpl* dst_port)
	: _queue(NULL)
	, _bufs(bufs)
	, _src_port(src_port)
	, _dst_port(dst_port)
	, _pending_disconnection(false)
{
	assert(src_port);
	assert(dst_port);
	assert(src_port != dst_port);
	assert(src_port->path() != dst_port->path());

	if (must_mix() || must_queue())
		_local_buffer = bufs.get(dst_port->type(), dst_port->buffer_size(), true);

	if (must_queue())
		_queue = new Raul::RingBuffer<LV2_Object>(src_port->buffer_size() * 2);
}


void
ConnectionImpl::dump() const
{
	debug << _src_port->path() << " -> " << _dst_port->path()
		<< (must_mix()   ? " (MIX) " : " (DIRECT) ")
		<< (must_queue() ? " (QUEUE)" : " (NOQUEUE) ")
		<< "POLY: " << _src_port->poly() << " => " << _dst_port->poly() << endl;
}


void
ConnectionImpl::update_buffer_size(Context& context, BufferFactory& bufs)
{
	if (must_mix() || must_queue())
		allocate_buffer(bufs);
}


void
ConnectionImpl::allocate_buffer(BufferFactory& bufs)
{
	if (!_local_buffer)
		_local_buffer = bufs.get(_dst_port->type(), _dst_port->buffer_size());
}


void
ConnectionImpl::prepare_poly(BufferFactory& bufs, uint32_t poly)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);

	assert(_src_port->prepared_poly() == _dst_port->prepared_poly()
			|| _src_port->prepared_poly() == 1
			|| _dst_port->prepared_poly() == 1);

	const bool mix = _src_port->prepared_poly() > _dst_port->prepared_poly();
	if ((mix || must_queue()) && !_local_buffer)
		_local_buffer = bufs.get(_dst_port->type(), _dst_port->buffer(0)->size());
}


void
ConnectionImpl::apply_poly(Raul::Maid& maid, uint32_t poly)
{
	ThreadManager::assert_thread(THREAD_PROCESS);

	assert(_src_port->poly() == _dst_port->poly()
			|| _src_port->poly() == 1
			|| _dst_port->poly() == 1);

	// Recycle buffer if it's no longer needed
	if (!(must_mix() || must_queue()))
		_local_buffer = NULL;
}


void
ConnectionImpl::process(Context& context)
{
	if (must_queue()) {
		IntrusivePtr<EventBuffer> src_buf = PtrCast<EventBuffer>(_src_port->buffer(0));
		if (!src_buf) {
			error << "Queued connection but source is not an EventBuffer" << endl;
			return;
		}

		IntrusivePtr<ObjectBuffer> local_buf = PtrCast<ObjectBuffer>(_local_buffer);
		if (!local_buf) {
			error << "Queued connection but local buffer is not an ObjectBuffer" << endl;
			return;
		}

		local_buf->clear();

		if (_queue->read_space()) {
			LV2_Object obj;
			_queue->full_peek(sizeof(LV2_Object), &obj);
			_queue->full_read(sizeof(LV2_Object) + obj.size, local_buf->object());
		}

	} else if (must_mix()) {
		const uint32_t num_srcs = src_port()->poly();
		Buffer* srcs[num_srcs];
		for (uint32_t v = 0; v < num_srcs; ++v)
			srcs[v] = src_port()->buffer(v).get();

		_local_buffer->clear();
		mix(context, _local_buffer.get(), srcs, num_srcs);
	}
}


void
ConnectionImpl::queue(Context& context)
{
	if (!must_queue())
		return;

	IntrusivePtr<EventBuffer> src_buf = PtrCast<EventBuffer>(_src_port->buffer(0));
	if (!src_buf) {
		error << "Queued connection but source is not an EventBuffer" << endl;
		return;
	}

	while (src_buf->is_valid()) {
		LV2_Event*  ev  = src_buf->get_event();
		LV2_Object* obj = LV2_OBJECT_FROM_EVENT(ev);
		/*debug << _src_port->path() << " -> " << _dst_port->path()
			<< " QUEUE OBJECT TYPE " << obj->type << ":";
		for (size_t i = 0; i < obj->size; ++i)
			debug << " " << std::hex << (int)obj->body[i];
		debug << endl;*/

		_queue->write(sizeof(LV2_Object) + obj->size, obj);
		src_buf->increment();

		context.engine().message_context()->run(_dst_port, context.start() + ev->frames);
	}
}


} // namespace Ingen

