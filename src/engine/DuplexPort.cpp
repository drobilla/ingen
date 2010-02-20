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

#include <cstdlib>
#include <cassert>
#include "shared/LV2URIMap.hpp"
#include "util.hpp"
#include "DuplexPort.hpp"
#include "ConnectionImpl.hpp"
#include "OutputPort.hpp"
#include "NodeImpl.hpp"
#include "ProcessContext.hpp"
#include "EventBuffer.hpp"

using namespace std;

namespace Ingen {

using namespace Shared;


DuplexPort::DuplexPort(
		BufferFactory&    bufs,
		NodeImpl*         parent,
		const string&     name,
		uint32_t          index,
		bool              polyphonic,
		uint32_t          poly,
		PortType          type,
		const Raul::Atom& value,
		size_t            buffer_size,
		bool              is_output)
	: PortImpl(bufs, parent, name, index, poly, type, value, buffer_size)
	, InputPort(bufs, parent, name, index, poly, type, value, buffer_size)
	, OutputPort(bufs, parent, name, index, poly, type, value, buffer_size)
	, _is_output(is_output)
{
	assert(PortImpl::_parent == parent);
	set_property(Shared::LV2URIMap::instance().ingen_polyphonic, polyphonic);
}


void
DuplexPort::get_buffers(BufferFactory& bufs, Raul::Array<BufferFactory::Ref>* buffers, uint32_t poly)
{
	if (_is_output)
		return InputPort::get_buffers(bufs, buffers, poly);
	else
		return OutputPort::get_buffers(bufs, buffers, poly);
}


/** Prepare for the execution of parent patch */
void
DuplexPort::pre_process(Context& context)
{
	// If we're a patch output, we're an input from the internal perspective.
	// Prepare buffers for write (so plugins can deliver to them)
	if (_is_output) {
		for (uint32_t v = 0; v < _poly; ++v)
			_buffers->at(v)->prepare_write(context);

	// If we're a patch input, were an output from the internal perspective.
	// Do whatever a normal node's input port does to prepare input for reading.
	} else {
		InputPort::pre_process(context);
	}
}


/** Finalize after the execution of parent patch (deliver outputs) */
void
DuplexPort::post_process(Context& context)
{
	// If we're a patch output, we're an input from the internal perspective.
	// Mix down input delivered by plugins so output (external perspective) is ready.
	if (_is_output) {
		InputPort::pre_process(context);

		if (_broadcast)
			broadcast_value(context, false);
	}
}


} // namespace Ingen

