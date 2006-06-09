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

#ifndef PORT_H
#define PORT_H

#include <cstdlib>
#include <string>
#include "util/types.h"
#include "OmObject.h"

using std::string;

namespace Om {

class Node;
class PortInfo;


/** A port on a Node.
 *
 * This is a non-template abstract base class, which basically exists so
 * things can pass around Port pointers and not have to worry about type,
 * templates, etc.
 *
 * \ingroup engine
 */
class Port : public OmObject
{
public:
	virtual ~Port() {}

	Port* as_port() { return this; }
	
	PortInfo* port_info() const       { return m_port_info; }
	void      port_info(PortInfo* pi) { m_port_info = pi; }
	
	void add_to_store();
	void remove_from_store();

	/** Called once per process cycle */
	virtual void prepare_buffers(size_t nframes) = 0;
	
	/** Empty buffer contents completely (ie silence) */
	virtual void clear_buffers() = 0;
	
	Node*  parent_node() const { return m_parent->as_node(); }
	bool   is_sample()   const { return false; }
	size_t num()         const { return m_index; }
	size_t poly()        const { return m_poly; }

protected:
	Port(Node* const node, const string& name, size_t index, size_t poly, PortInfo* port_info);
	
	// Prevent copies (undefined)
	Port(const Port&);
	Port& operator=(const Port&);

	size_t    m_index;
	size_t    m_poly;
	PortInfo* m_port_info;
};


} // namespace Om

#endif // PORT_H
