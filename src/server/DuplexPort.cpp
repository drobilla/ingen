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
#include "DuplexPort.hpp"
#include "OutputPort.hpp"
#include "PatchImpl.hpp"

using namespace std;

namespace Ingen {
namespace Server {

DuplexPort::DuplexPort(BufferFactory&      bufs,
                       NodeImpl*           parent,
                       const Raul::Symbol& symbol,
                       uint32_t            index,
                       bool                polyphonic,
                       uint32_t            poly,
                       PortType            type,
                       LV2_URID            buffer_type,
                       const Raul::Atom&   value,
                       size_t              buffer_size,
                       bool                is_output)
	: PortImpl(bufs, parent, symbol, index, poly, type, buffer_type, value, buffer_size)
	, InputPort(bufs, parent, symbol, index, poly, type, buffer_type, value, buffer_size)
	, OutputPort(bufs, parent, symbol, index, poly, type, buffer_type, value, buffer_size)
	, _is_output(is_output)
{
	set_property(bufs.uris().ingen_polyphonic,
	             bufs.forge().make(polyphonic));
}

DuplexPort::~DuplexPort()
{
	if (is_linked()) {
		parent_patch()->remove_port(*this);
	}
}

bool
DuplexPort::get_buffers(BufferFactory&          bufs,
                        Raul::Array<BufferRef>* buffers,
                        uint32_t                poly,
                        bool                    real_time) const
{
	if (_is_output) {
		return InputPort::get_buffers(bufs, buffers, poly, real_time);
	} else {
		return OutputPort::get_buffers(bufs, buffers, poly, real_time);
	}
}

uint32_t
DuplexPort::max_tail_poly(Context& context) const
{
	return parent_patch()->internal_poly_process();
}

/** Prepare for the execution of parent patch */
void
DuplexPort::pre_process(Context& context)
{
	if (_is_output) {
		/* This is a patch output, which is an input from the internal
		   perspective.  Prepare buffers for write so plugins can deliver to
		   them */
		for (uint32_t v = 0; v < _poly; ++v) {
			_buffers->at(v)->prepare_write(context);
		}
	} else {
		/* This is a a patch input, which is an output from the internal
		   perspective.  Do whatever a normal node's input port does to prepare
		   input for reading. */
		InputPort::pre_process(context);
	}
}

/** Finalize after the execution of parent patch (deliver outputs) */
void
DuplexPort::post_process(Context& context)
{
	if (_is_output) {
		/* This is a patch output, which is an input from the internal
		   perspective.  Mix down input delivered by plugins so output
		   (external perspective) is ready. */
		InputPort::pre_process(context);

		if (_broadcast)
			broadcast_value(context, false);
	}
}

} // namespace Server
} // namespace Ingen
