/*
  This file is part of Ingen.
  Copyright 2007-2016 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_ENGINE_OUTPUTPORT_HPP
#define INGEN_ENGINE_OUTPUTPORT_HPP

#include "PortImpl.hpp"

namespace Ingen {
namespace Server {

/** An output port.
 *
 * Output ports always have a locally allocated buffer, and buffer() will
 * always return that buffer.
 *
 * \ingroup engine
 */
class OutputPort : public PortImpl
{
public:
	OutputPort(BufferFactory&      bufs,
	           BlockImpl*          parent,
	           const Raul::Symbol& symbol,
	           uint32_t            index,
	           uint32_t            poly,
	           PortType            type,
	           LV2_URID            buffer_type,
	           const Atom&         value,
	           size_t              buffer_size = 0)
		: PortImpl(bufs, parent, symbol, index,poly, type, buffer_type, value, buffer_size, true)
	{}
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_OUTPUTPORT_HPP
