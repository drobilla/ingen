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

#ifndef INGEN_ENGINE_INPUTPORT_HPP
#define INGEN_ENGINE_INPUTPORT_HPP

#include <string>
#include <cstdlib>
#include <cassert>
#include "raul/List.hpp"
#include "raul/SharedPtr.hpp"
#include "PortImpl.hpp"

namespace Ingen {

class ConnectionImpl;
class Context;
class NodeImpl;
class OutputPort;
class ProcessContext;


/** An input port on a Node or Patch.
 *
 * All ports have a Buffer, but the actual contents (data) of that buffer may be
 * set directly to the incoming connection's buffer if there's only one inbound
 * connection, to eliminate the need to copy/mix.
 *
 * If a port has multiple connections, they will be mixed down into the local
 * buffer and it will be used.
 *
 * \ingroup engine
 */
class InputPort : virtual public PortImpl
{
public:
	InputPort(BufferFactory&      bufs,
	          NodeImpl*           parent,
	          const Raul::Symbol& symbol,
	          uint32_t            index,
	          uint32_t            poly,
	          Shared::PortType    type,
	          const Raul::Atom&   value,
	          size_t              buffer_size=0);

	virtual ~InputPort() {}

	typedef Raul::List< SharedPtr<ConnectionImpl> > Connections;

	void               add_connection(Connections::Node* c);
	Connections::Node* remove_connection(ProcessContext& context, const OutputPort* src_port);

	bool apply_poly(Raul::Maid& maid, uint32_t poly);
	void set_buffer_size(Context& context, BufferFactory& bufs, size_t size);

	void get_buffers(BufferFactory& bufs, Raul::Array<BufferFactory::Ref>* buffers, uint32_t poly);

	void pre_process(Context& context);
	void post_process(Context& context);

	size_t num_connections() const { return _num_connections; } ///< Pre-process thread
	void increment_num_connections() { ++_num_connections; }
	void decrement_num_connections() { --_num_connections; }

	bool is_input()  const { return true; }
	bool is_output() const { return false; }

protected:
	size_t      _num_connections; ///< Pre-process thread
	Connections _connections;
};


} // namespace Ingen

#endif // INGEN_ENGINE_INPUTPORT_HPP
