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

#ifndef INGEN_ENGINE_OUTPUTPORT_HPP
#define INGEN_ENGINE_OUTPUTPORT_HPP

#include <cstdlib>

#include "PortImpl.hpp"

namespace Ingen {
namespace Server {

/** An output port.
 *
 * Output ports always have a locally allocated buffer, and buffer() will
 * always return that buffer.  (This is very different from InputPort)
 *
 * \ingroup engine
 */
class OutputPort : virtual public PortImpl
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
	           size_t              buffer_size = 0);

	virtual ~OutputPort() {}

	bool get_buffers(BufferFactory&      bufs,
	                 Raul::Array<Voice>* voices,
	                 uint32_t            poly,
	                 bool                real_time) const;

	void pre_process(RunContext& context);
	void post_process(RunContext& context);

	SampleCount next_value_offset(SampleCount offset, SampleCount end) const;
	void        update_values(SampleCount offset, uint32_t voice);

	bool is_input()  const { return false; }
	bool is_output() const { return true; }
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_OUTPUTPORT_HPP
