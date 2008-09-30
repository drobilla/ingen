/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#ifndef OUTPUTPORT_H
#define OUTPUTPORT_H

#include <string>
#include <cstdlib>
#include "PortImpl.hpp"
#include "types.hpp"

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
	OutputPort(NodeImpl*          parent,
	           const std::string& name,
	           uint32_t           index,
	           uint32_t           poly,
	           DataType           type,
	           const Atom&        value,
	           size_t             buffer_size);

	void pre_process(ProcessContext& context);
	void post_process(ProcessContext& context);
	
	virtual ~OutputPort() {}

	bool is_input()  const { return false; }
	bool is_output() const { return true; }
};


} // namespace Ingen

#endif // OUTPUTPORT_H
