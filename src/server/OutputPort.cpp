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

#include "ingen/URIs.hpp"

#include "Buffer.hpp"
#include "BufferFactory.hpp"
#include "Engine.hpp"
#include "BlockImpl.hpp"
#include "OutputPort.hpp"

using namespace std;

namespace Ingen {
namespace Server {

OutputPort::OutputPort(BufferFactory&      bufs,
                       BlockImpl*          parent,
                       const Raul::Symbol& symbol,
                       uint32_t            index,
                       uint32_t            poly,
                       PortType            type,
                       LV2_URID            buffer_type,
                       const Raul::Atom&   value,
                       size_t              buffer_size)
	: PortImpl(bufs, parent, symbol, index, poly, type, buffer_type, value, buffer_size)
{
	if (parent->graph_type() != Node::GraphType::GRAPH) {
		add_property(bufs.uris().rdf_type, bufs.uris().lv2_OutputPort);
	}

	_broadcast = true;

	setup_buffers(bufs, poly, false);
}

bool
OutputPort::get_buffers(BufferFactory&          bufs,
                        Raul::Array<BufferRef>* buffers,
                        uint32_t                poly,
                        bool                    real_time) const
{
	for (uint32_t v = 0; v < poly; ++v)
		buffers->at(v) = bufs.get_buffer(buffer_type(), _buffer_size, real_time);

	return true;
}

void
OutputPort::pre_process(Context& context)
{
	for (uint32_t v = 0; v < _poly; ++v)
		_buffers->at(v)->prepare_output_write(context);
}

void
OutputPort::post_process(Context& context)
{
	for (uint32_t v = 0; v < _poly; ++v) {
		update_set_state(context, v);
	}

	if (_broadcast)
		broadcast_value(context, false);
}

} // namespace Server
} // namespace Ingen
