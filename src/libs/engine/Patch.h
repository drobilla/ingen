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

#ifndef PATCH_H
#define PATCH_H

#include <cstdlib>
#include <string>
#include "NodeBase.h"
#include "Plugin.h"
#include "List.h"
#include "DataType.h"

using std::string;

template <typename T> class Array;

namespace Om {

class Connection;
class InternalNode;
namespace Shared {
	class ClientInterface;
} using Shared::ClientInterface;


/** A group of nodes in a graph, possibly polyphonic.
 *
 * Note that this is also a Node, just one which contains Nodes.
 * Therefore infinite subpatching is possible, of polyphonic
 * patches of polyphonic nodes etc. etc.
 *
 * \ingroup engine
 */
class Patch : public NodeBase
{
public:
	Patch(const string& name, size_t poly, Patch* parent, samplerate srate, size_t buffer_size, size_t local_poly);
	virtual ~Patch();

	Patch* as_patch() { return static_cast<Patch*>(this); }

	void activate();
	void deactivate();

	void run(size_t nframes);
	
	size_t num_ports() const;

	//void send_creation_messages(ClientInterface* client) const;
	
	void add_to_store();
	void remove_from_store();
	
	void set_path(const Path& new_path);
	
	// Patch specific stuff not inherited from Node
	
	void             add_node(ListNode<Node*>* tn);
	ListNode<Node*>* remove_node(const string& name);

	List<Node*>&       nodes()       { return _nodes; }
	List<Connection*>& connections() { return _connections; }
	
	const List<Node*>&       nodes()       const { return _nodes; }
	const List<Connection*>& connections() const { return _connections; }
	
	//void                     add_bridge_node(ListNode<InternalNode*>* n) { _bridge_nodes.push_back(n); }
	//ListNode<InternalNode*>* remove_bridge_node(const InternalNode* n);
	Port*            create_port(const string& name, DataType type, size_t buffer_size, bool is_output);
	void             add_port(ListNode<Port*>* port) { _patch_ports.push_back(port); }
	ListNode<Port*>* remove_port(const Port* p);
	
	void                   add_connection(ListNode<Connection*>* c) { _connections.push_back(c); }
	ListNode<Connection*>* remove_connection(const Port* src_port, const Port* dst_port);
	
	Array<Node*>* process_order()                 { return _process_order; }
	void          process_order(Array<Node*>* po) { _process_order = po; }
	
	Array<Port*>* external_ports()                 { return _ports; }
	void          external_ports(Array<Port*>* pa) { _ports = pa; }

	Array<Node*>* build_process_order() const;
	inline void   build_process_order_recursive(Node* n, Array<Node*>* order) const;
	
	/** Whether to run this patch's DSP in the audio thread */
	bool process() const { return _process; }
	void process(bool p);

	size_t internal_poly() const { return _internal_poly; }
	
	const Plugin* plugin() const              { return &_plugin; }
	void          plugin(const Plugin* const) { exit(EXIT_FAILURE); }

private:
	// Prevent copies (undefined)
	Patch(const Patch&);
	Patch& operator=(const Patch&);

	size_t             _internal_poly;
	Array<Node*>*      _process_order; ///< Accessed in audio thread only
	List<Connection*>  _connections;   ///< Accessed in audio thread only
	List<Port*>        _patch_ports;   ///< Accessed in preprocessing thread only
	List<Node*>        _nodes;         ///< Accessed in preprocessing thread only
	bool               _process;

	Plugin _plugin;
};



/** Private helper for build_process_order */
inline void
Patch::build_process_order_recursive(Node* n, Array<Node*>* order) const
{
	if (n == NULL || n->traversed()) return;
	n->traversed(true);
	assert(order != NULL);
	
	for (List<Node*>::iterator i = n->providers()->begin(); i != n->providers()->end(); ++i)
		if ( ! (*i)->traversed() )
			build_process_order_recursive((*i), order);

	order->push_back(n);
}


} // namespace Om

#endif // PATCH_H
