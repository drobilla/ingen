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

#ifndef PATCH_H
#define PATCH_H

#include <cstdlib>
#include <string>
#include <raul/List.hpp>
#include "NodeBase.hpp"
#include "Plugin.hpp"
#include "DataType.hpp"

using std::string;

template <typename T> class Array;

namespace Ingen {

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
	Patch(const string& name, size_t poly, Patch* parent, SampleRate srate, size_t buffer_size, size_t local_poly);
	virtual ~Patch();

	void activate();
	void deactivate();

	void process(SampleCount nframes, FrameTime start, FrameTime end);
	
	void set_buffer_size(size_t size);
	
	void add_to_store(ObjectStore* store);
	void remove_from_store();
	
	void set_path(const Path& new_path);
	
	// Patch specific stuff not inherited from Node
	
	void                   add_node(Raul::ListNode<Node*>* tn);
	Raul::ListNode<Node*>* remove_node(const string& name);

	Raul::List<Node*>&       nodes()       { return _nodes; }
	Raul::List<Connection*>& connections() { return _connections; }
	
	const Raul::List<Node*>&       nodes()       const { return _nodes; }
	const Raul::List<Connection*>& connections() const { return _connections; }
	
	size_t num_ports() const;
	
	Port* create_port(const string& name, DataType type, size_t buffer_size, bool is_output);
	void add_input(Raul::ListNode<Port*>* port)  { _input_ports.push_back(port); } ///< Preprocesser thread
	void add_output(Raul::ListNode<Port*>* port) { _output_ports.push_back(port); } ///< Preprocessor thread
	Raul::ListNode<Port*>* remove_port(const string& name);
	
	void add_connection(Raul::ListNode<Connection*>* c) { _connections.push_back(c); }
	Raul::ListNode<Connection*>* remove_connection(const Port* src_port, const Port* dst_port);
	
	Raul::Array<Node*>* process_order()                       { return _process_order; }
	void                process_order(Raul::Array<Node*>* po) { _process_order = po; }
	
	Raul::Array<Port*>* external_ports()                       { return _ports; }
	void                external_ports(Raul::Array<Port*>* pa) { _ports = pa; }

	Raul::Array<Node*>* build_process_order() const;
	Raul::Array<Port*>* build_ports_array() const;
	
	/** Whether to run this patch's DSP bits in the audio thread */
	bool enabled() const { return _process; }
	void enable() { _process = true; }
	void disable();

	size_t internal_poly() const { return _internal_poly; }

private:
	inline void build_process_order_recursive(Node* n, Raul::Array<Node*>* order) const;

	size_t                  _internal_poly;
	Raul::Array<Node*>*     _process_order; ///< Accessed in audio thread only
	Raul::List<Connection*> _connections;   ///< Accessed in audio thread only
	Raul::List<Port*>       _input_ports;   ///< Accessed in preprocessing thread only
	Raul::List<Port*>       _output_ports;  ///< Accessed in preprocessing thread only
	Raul::List<Node*>       _nodes;         ///< Accessed in preprocessing thread only
	bool                    _process;
};



/** Private helper for build_process_order */
inline void
Patch::build_process_order_recursive(Node* n, Raul::Array<Node*>* order) const
{
	if (n == NULL || n->traversed()) return;
	n->traversed(true);
	assert(order != NULL);
	
	for (Raul::List<Node*>::iterator i = n->providers()->begin(); i != n->providers()->end(); ++i)
		if ( ! (*i)->traversed() )
			build_process_order_recursive((*i), order);

	order->push_back(n);
}


} // namespace Ingen

#endif // PATCH_H
