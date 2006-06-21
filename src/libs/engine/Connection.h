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

#ifndef CONNECTION_H
#define CONNECTION_H

#include <cstdlib>
#include "MaidObject.h"
#include "types.h"

namespace Om {

class Port;


/** Represents a single inbound connection for an InputPort.
 *
 * This can be a group of ports (ie coming from a polyphonic Node) or
 * a single Port.  This class exists basically as an abstraction of mixing
 * down polyphonic inputs, so InputPort can just deal with mixing down
 * multiple connections (oblivious to the polyphonic situation of the
 * connection itself).
 *
 * \ingroup engine
 */
class Connection : public MaidObject
{
public:
	virtual ~Connection() {}
	
	Port* src_port() const { return m_src_port; }
	Port* dst_port() const { return m_dst_port; }

	/** Used by some (recursive) events to prevent double disconnections */
	bool pending_disconnection()       { return m_pending_disconnection; }
	void pending_disconnection(bool b) { m_pending_disconnection = b; }
	
protected:
	// Disallow copies (undefined)
	Connection(const Connection&);

	Connection(Port* const src_port, Port* const dst_port);
	
	Port* const m_src_port;
	Port* const m_dst_port;
	bool        m_pending_disconnection;
};


} // namespace Om

#endif // CONNECTION_H
