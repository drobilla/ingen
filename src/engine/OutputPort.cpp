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

#include "interface/Patch.hpp"
#include "shared/LV2URIMap.hpp"
#include "Buffer.hpp"
#include "NodeImpl.hpp"
#include "OutputPort.hpp"
#include "ProcessContext.hpp"

using namespace std;

namespace Ingen {

namespace Shared { class Patch; }
using namespace Shared;

OutputPort::OutputPort(BufferFactory&      bufs,
                       NodeImpl*           parent,
                       const Raul::Symbol& symbol,
                       uint32_t            index,
                       uint32_t            poly,
                       PortType            type,
                       const Raul::Atom&   value,
                       size_t              buffer_size)
	: PortImpl(bufs, parent, symbol, index, poly, type, value, buffer_size)
{
	const LV2URIMap& uris = Shared::LV2URIMap::instance();

	if (!dynamic_cast<Patch*>(parent))
		add_property(uris.rdf_type, uris.lv2_OutputPort);

	if (type == PortType::CONTROL)
		_broadcast = true;

	setup_buffers(bufs, poly);
}


void
OutputPort::get_buffers(BufferFactory& bufs, Raul::Array<BufferFactory::Ref>* buffers, uint32_t poly)
{
	for (uint32_t v = 0; v < poly; ++v)
		buffers->at(v) = bufs.get(_type, _buffer_size);
}


void
OutputPort::pre_process(Context& context)
{
	for (uint32_t v = 0; v < _poly; ++v)
		_buffers->at(v)->prepare_write(context);
}


void
OutputPort::post_process(Context& context)
{
	for (uint32_t v = 0; v < _poly; ++v)
		_buffers->at(v)->prepare_read(context);

	if (_broadcast)
		broadcast_value(context, false);
}


} // namespace Ingen
