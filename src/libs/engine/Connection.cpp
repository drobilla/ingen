/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Connection.h"
#include "util.h"
#include "Node.h"
#include "Om.h"
#include "Port.h"

namespace Om {


/** Constructor for a connection from a node's output port.
 *
 * This handles both polyphonic and monophonic nodes, transparently to the 
 * user (InputPort).
 */
Connection::Connection(Port* const src_port, Port* const dst_port)
: m_src_port(src_port),
  m_dst_port(dst_port),
  m_pending_disconnection(false)
{
	assert(src_port != NULL);
	assert(dst_port != NULL);

	assert((src_port->parent_node()->poly() == dst_port->parent_node()->poly())
		|| (src_port->parent_node()->poly() == 1 || dst_port->parent_node()->poly() == 1));
}

} // namespace Om

