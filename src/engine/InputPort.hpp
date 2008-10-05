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

#ifndef INPUTPORT_H
#define INPUTPORT_H

#include <string>
#include <cstdlib>
#include <cassert>
#include <raul/List.hpp>
#include <raul/SharedPtr.hpp>
#include "PortImpl.hpp"
using std::string;

namespace Ingen {

class ConnectionImpl;
class OutputPort;
class NodeImpl;


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
	InputPort(NodeImpl*     parent,
	          const string& name,
	          uint32_t      index,
	          uint32_t      poly,
	          DataType      type,
	          const Atom&   value,
	          size_t        buffer_size);

	virtual ~InputPort() {}
	
	typedef Raul::List< SharedPtr<ConnectionImpl> > Connections;
	
	void               add_connection(Connections::Node* c);
	Connections::Node* remove_connection(const OutputPort* src_port);

	const Connections& connections() { return _connections; }
	
	bool prepare_poly(uint32_t poly);
	bool apply_poly(Raul::Maid& maid, uint32_t poly);

	void pre_process(ProcessContext& context);
	void post_process(ProcessContext& context);
	
	bool is_connected() const { return (_connections.size() > 0); }
	
	bool is_input()  const { return true; }
	bool is_output() const { return false; }
	
	virtual void set_buffer_size(size_t size);
	
private:
	Connections _connections;
};


} // namespace Ingen

#endif // INPUTPORT_H
