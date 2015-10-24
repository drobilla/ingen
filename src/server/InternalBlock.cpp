/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Buffer.hpp"
#include "InternalBlock.hpp"
#include "PortImpl.hpp"

namespace Ingen {
namespace Server {

InternalBlock::InternalBlock(PluginImpl*         plugin,
                             const Raul::Symbol& symbol,
                             bool                poly,
                             GraphImpl*          parent,
                             SampleRate          rate)
	: BlockImpl(plugin, symbol, poly, parent, rate)
{}


void
InternalBlock::pre_process(ProcessContext& context) {
	/* Output sequences are initialized in LV2 format, an atom:Chunk with size
	   set to the capacity of the buffer.  Internal nodes don't care, so clear
	   to an empty sequences so appending events results in a valid output. */
	for (uint32_t i = 0; i < num_ports(); ++i) {
		PortImpl* const port = _ports->at(i);
		if (port->is_input()) {
			port->pre_process(context);
		} else {
			for (uint32_t v = 0; v < port->poly(); ++v) {
				port->buffer(v)->clear();
			}
		}
	}
}

} // namespace Server
} // namespace Ingen
