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

#ifndef BRIDGENODE_H
#define BRIDGENODE_H

#include <string>
#include "InternalNode.h"
#include "PortBase.h"
using std::string;

template <typename T> class ListNode;

namespace Om {

class DriverPort;
namespace Shared {
	class ClientInterface;
} using Shared::ClientInterface;


/** A Node to represent input/output Ports on a Patch.
 *
 * This node acts as both a Node and a Port (it is the only Node type that
 * returns non-null for as_port()).  The node is a normal Node in a Patch,
 * the Port is a Port on that Patch (port->parent == node->parent).
 * 
 * This class handles all DriverPort functionality as well (if this Node
 * is on a top level Patch).
 *
 * Both input and output nodes are handled in this class.
 *
 * This node will force itself to monophonic (regardless of the poly parameter
 * passed to constructor) if the parent of the patch it's representing a port
 * on has a different polyphony than the patch (since connecting mismatched
 * polyphonies is impossible).
 *
 * \ingroup engine
 */
template <typename T>
class BridgeNode : public InternalNode
{
public:
	virtual ~BridgeNode();
	
	Port* as_port() { return m_external_port; }
	
	void activate();
	void deactivate();
	void add_to_patch();
	void remove_from_patch();
	//void send_creation_messages(ClientInterface* client) const;

	void set_path(const Path& new_path);
	
protected:	
	// Disallow copies (undefined)
	BridgeNode(const BridgeNode&);
	BridgeNode& operator=(const BridgeNode&);
	
	BridgeNode(const string& path, size_t poly, Patch* parent, samplerate srate, size_t buffer_size);

	/** Driver port, used if this is on a top level Patch */
	DriverPort*  m_driver_port;
	
	ListNode<InternalNode*>* m_listnode;
	
	PortBase<T>*  m_external_port;
};


template class BridgeNode<sample>;
template class BridgeNode<MidiMessage>;

} // namespace Om

#endif // BRIDGENODE_H
