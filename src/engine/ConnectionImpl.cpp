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
#include "raul/Maid.hpp"
#include "util.hpp"
#include "ConnectionImpl.hpp"
#include "PortImpl.hpp"
#include "AudioBuffer.hpp"
#include "ProcessContext.hpp"
#include "InputPort.hpp"
#include "BufferFactory.hpp"

namespace Ingen {

using namespace Shared;

/** Constructor for a connection from a node's output port.
 *
 * This handles both polyphonic and monophonic nodes, transparently to the
 * user (InputPort).
 */
ConnectionImpl::ConnectionImpl(BufferFactory& bufs, PortImpl* src_port, PortImpl* dst_port)
	: _bufs(bufs)
	, _src_port(src_port)
	, _dst_port(dst_port)
	, _pending_disconnection(false)
{
	assert(src_port);
	assert(dst_port);
	assert(src_port != dst_port);
	assert(src_port->path() != dst_port->path());
	assert(src_port->type() == dst_port->type()
			|| ( (src_port->type() == DataType::CONTROL || src_port->type() == DataType::AUDIO)
				&& (dst_port->type() == DataType::CONTROL || dst_port->type() == DataType::AUDIO) ));

	/*assert((src_port->parent_node()->poly() == dst_port->parent_node()->poly())
		|| (src_port->parent_node()->poly() == 1 || dst_port->parent_node()->poly() == 1));*/

	if (must_mix())
		_local_buffer = bufs.get(dst_port->type(), dst_port->buffer_size());
}


void
ConnectionImpl::dump() const
{
	cerr << _src_port->path() << " -> " << _dst_port->path()
		<< (must_mix() ? " MIX" : " DIRECT") << endl;
}


void
ConnectionImpl::set_buffer_size(BufferFactory& bufs, size_t size)
{
	if (must_mix())
		_local_buffer = bufs.get(_dst_port->type(), _dst_port->buffer(0)->size());
}


void
ConnectionImpl::prepare_poly(BufferFactory& bufs, uint32_t poly)
{
	_src_port->prepare_poly(bufs, poly);

	if (must_mix())
		_local_buffer = bufs.get(_dst_port->type(), _dst_port->buffer(0)->size());
}


void
ConnectionImpl::apply_poly(Raul::Maid& maid, uint32_t poly)
{
	_src_port->apply_poly(maid, poly);

	// Recycle buffer if it's no longer needed
	if (!must_mix() && _local_buffer)
		_local_buffer.reset();
}


void
ConnectionImpl::process(Context& context)
{
	if (!must_mix())
		return;

	// Copy the first voice
	_local_buffer->copy(context, src_port()->buffer(0).get());

	// Mix in the rest
	for (uint32_t v = 0; v < src_port()->poly(); ++v)
		_local_buffer->mix(context, src_port()->buffer(v).get());
}


} // namespace Ingen

