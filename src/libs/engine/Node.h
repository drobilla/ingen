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

#ifndef NODE_H
#define NODE_H

#include <string>
#include <raul/Array.h>
#include "types.h"
#include "GraphObject.h"


using std::string;

namespace Raul { template <typename T> class List; }

namespace Ingen {

class Buffer;
class Plugin;
class Patch;
class Port;
class OutputPort;
namespace Shared { class ClientInterface; }


/** A Node (or "module") in a Patch (which is also a Node).
 * 
 * A Node is a unit with input/output ports, a process() method, and some other
 * things.
 * 
 * This is a pure abstract base class for any Node, it contains no
 * implementation details/data whatsoever.  This is the interface you need to
 * implement to add a new Node type.
 *
 * \ingroup engine
 */
class Node : public GraphObject
{
public:
	Node(GraphObject* parent, const string& name) : GraphObject(parent, name) {}
	virtual ~Node() {}

	/** Activate this Node.
	 *
	 * This function will be called in a non-realtime thread before it is
	 * inserted in to a patch.  Any non-realtime actions that need to be
	 * done before the Node is ready for use should be done here.
	 */
	virtual void activate()   = 0;
	virtual void deactivate() = 0;
	virtual bool activated()  = 0;
	
	/** Run the node for @a nframes input/output.
	 *
	 * @a start and @a end are transport times: end is not redundant in the case
	 * of varispeed, where end-start != nframes.
	 */
	virtual void process(SampleCount nframes, FrameTime start, FrameTime end) = 0;

	virtual void set_port_buffer(size_t voice, size_t port_num, Buffer* buf) = 0;

	// FIXME: Only used by client senders.  Remove?
	virtual const Raul::Array<Port*>& ports() const = 0;
	
	virtual size_t num_ports() const  = 0;
	virtual size_t poly() const       = 0;
	
	/** Used by the process order finding algorithm (ie during connections) */
	virtual bool traversed() const  = 0;
	virtual void traversed(bool b)  = 0;
	
	/** Nodes that are connected to this Node's inputs.
	 * (This Node depends on them)
	 */
	virtual Raul::List<Node*>* providers()                     = 0;
	virtual void               providers(Raul::List<Node*>* l) = 0;
	
	/** Nodes are are connected to this Node's outputs.
	 * (They depend on this Node)
	 */
	virtual Raul::List<Node*>* dependants()                     = 0;
	virtual void               dependants(Raul::List<Node*>* l) = 0;
	
	/** The Patch this Node belongs to. */
	virtual Patch* parent_patch() const = 0;

	/** Information about the Plugin this Node is an instance of.
	 * Not the best name - not all nodes come from plugins (ie Patch)
	 */
	virtual const Plugin* plugin() const = 0;

	virtual void set_buffer_size(size_t size) = 0;
};


} // namespace Ingen

#endif // NODE_H
