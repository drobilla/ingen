/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#ifndef INGEN_ENGINE_OUTPUTPORT_HPP
#define INGEN_ENGINE_OUTPUTPORT_HPP

#include <string>
#include <cstdlib>
#include "PortImpl.hpp"

namespace Ingen {


/** An output port.
 *
 * Output ports always have a locally allocated buffer, and buffer() will
 * always return that buffer.  (This is very different from InputPort)
 *
 * This class actually adds no functionality to Port whatsoever right now,
 * it will in the future when more advanced port types exist, and it makes
 * things clearer throughout the engine.
 *
 * \ingroup engine
 */
class OutputPort : virtual public PortImpl
{
public:
	OutputPort(BufferFactory&      bufs,
	           NodeImpl*           parent,
	           const Raul::Symbol& symbol,
	           uint32_t            index,
	           uint32_t            poly,
	           Shared::PortType    type,
	           const Raul::Atom&   value,
	           size_t              buffer_size=0);

	bool get_buffers(BufferFactory& bufs, Raul::Array<BufferFactory::Ref>* buffers, uint32_t poly);

	void pre_process(Context& context);
	void post_process(Context& context);

	virtual ~OutputPort() {}

	bool is_input()  const { return false; }
	bool is_output() const { return true; }
};


} // namespace Ingen

#endif // INGEN_ENGINE_OUTPUTPORT_HPP
