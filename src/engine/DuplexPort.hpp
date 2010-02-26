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

#ifndef INGEN_ENGINE_DUPLEXPORT_HPP
#define INGEN_ENGINE_DUPLEXPORT_HPP

#include <string>
#include "Buffer.hpp"
#include "InputPort.hpp"
#include "OutputPort.hpp"

namespace Ingen {

class NodeImpl;


/** A duplex port (which is both an InputPort and an OutputPort)
 *
 * This is used for Patch ports, since they need to appear as both an input
 * and an output port based on context.  Eg. a patch output appears as an
 * input inside the patch, so nodes inside the patch can feed it data.
 *
 * \ingroup engine
 */
class DuplexPort : public InputPort, public OutputPort
{
public:
	DuplexPort(BufferFactory&     bufs,
	           NodeImpl*          parent,
	           const std::string& name,
	           uint32_t           index,
	           bool               polyphonic,
	           uint32_t           poly,
	           Shared::PortType   type,
	           const Raul::Atom&  value,
	           size_t             buffer_size,
	           bool               is_output);

	virtual ~DuplexPort() {}

	bool get_buffers(BufferFactory& bufs, Raul::Array<BufferFactory::Ref>* buffers, uint32_t poly);

	void pre_process(Context& context);
	void post_process(Context& context);

	bool is_input()  const { return !_is_output; }
	bool is_output() const { return _is_output; }

protected:
	bool _is_output;
};


} // namespace Ingen

#endif // INGEN_ENGINE_DUPLEXPORT_HPP
