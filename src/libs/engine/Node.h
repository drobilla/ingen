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

#ifndef NODE_H
#define NODE_H

#include <string>
#include "util/types.h"
#include "OmObject.h"
#include "Array.h"

using std::string;

template <typename T> class List;

namespace Om {

class Port;
template <typename T> class OutputPort;
class Plugin;
class Patch;
namespace Shared {
	class ClientInterface;
}


/** A Node (or "module") in a Patch (which is also a Node).
 * 
 * A Node is a unit with input/output ports, a run() method, and some other
 * things.
 * 
 * This is a pure abstract base class for any Node, it contains no
 * implementation details/data whatsoever.  This is the interface you need to
 * implement to add a new Node type to Om.
 *
 * \ingroup engine
 */
class Node : public OmObject
{
public:
	Node(OmObject* parent, const string& name) : OmObject(parent, name) {}
	virtual ~Node() {}

	Node* as_node() { return static_cast<Node*>(this); }

	/** Activate this Node.
	 *
	 * This function will be called in a non-realtime thread before it is
	 * inserted in to a patch.  Any non-realtime actions that need to be
	 * done before the Node is ready for use should be done here.
	 */
	virtual void activate()   = 0;
	virtual void deactivate() = 0;
	virtual bool activated()  = 0;

	virtual void run(size_t nframes) = 0;
		
	virtual void set_port_buffer(size_t voice, size_t port_num, void* buf) = 0;

	// FIXME: Only used by client senders.  Remove?
	virtual const Array<Port*>& ports() const = 0;
	
	virtual size_t num_ports() const  = 0;
	virtual size_t poly() const       = 0;
	
	/** Used by the process order finding algorithm (ie during connections) */
	virtual bool traversed() const  = 0;
	virtual void traversed(bool b)  = 0;
	
	/** Nodes that are connected to this Node's inputs.
	 * (This Node depends on them)
	 */
	virtual List<Node*>* providers()               = 0;
	virtual void         providers(List<Node*>* l) = 0;
	
	/** Nodes are are connected to this Node's outputs.
	 * (They depend on this Node)
	 */
	virtual List<Node*>* dependants()               = 0;
	virtual void         dependants(List<Node*>* l) = 0;
	
	/** The Patch this Node belongs to. */
	virtual Patch* parent_patch() const   = 0;

	/** Information about what 'plugin' this Node is an instance of.
	 * Not the best name - not all nodes come from plugins (ie Patch)
	 */
	virtual const Plugin* plugin() const = 0;

	/** Add self to a Patch.
	 *
	 * This function must be realtime-safe!  Any non-realtime actions that
	 * need to be done before adding to a patch can be done in activate().
	 */
	virtual void add_to_patch() = 0;

	virtual void remove_from_patch() = 0;

	/** Send any necessary notification to client on node creation. */
	//virtual void send_creation_messages(Shared::ClientInterface* client) const = 0;
};


} // namespace Om

#endif // NODE_H
