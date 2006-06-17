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

#include <cassert>
#include <cmath>
#include <iostream>
#include "Node.h"
#include "Patch.h"
#include "Plugin.h"
#include "Port.h"
#include "PortInfo.h"
#include "ClientBroadcaster.h"
#include "InternalNode.h"
#include "Connection.h"
#include "Om.h"
#include "OmApp.h"
#include "PortBase.h"
#include "ObjectStore.h"
#include "interface/ClientInterface.h"

using std::cerr; using std::cout; using std::endl;

namespace Om {


Patch::Patch(const string& path, size_t poly, Patch* parent, samplerate srate, size_t buffer_size, size_t internal_poly) 
: NodeBase(path, poly, parent, srate, buffer_size),
  m_internal_poly(internal_poly),
  m_process_order(NULL),
  m_process(false)
{
	assert(internal_poly >= 1);

	m_plugin.type(Plugin::Patch);
	m_plugin.uri("http://codeson.net/grauph/patch");
	m_plugin.plug_label("om_patch");
	m_plugin.name("Om patch");

	//std::cerr << "Creating patch " << m_name << ", poly = " << poly
	//	<< ", internal poly = " << internal_poly << std::endl;
}


Patch::~Patch()
{
	assert(!m_activated);
	
	for (List<Connection*>::iterator i = m_connections.begin(); i != m_connections.end(); ++i) {
		delete (*i);
		delete m_connections.remove(i);
	}

	for (List<Node*>::iterator i = m_nodes.begin(); i != m_nodes.end(); ++i) {
		assert(!(*i)->activated());
		delete (*i);
		delete m_nodes.remove(i);
	}

	delete m_process_order;
}


void
Patch::activate()
{
	NodeBase::activate();

	for (List<Node*>::iterator i = m_nodes.begin(); i != m_nodes.end(); ++i)
		(*i)->activate();
	
	assert(m_activated);
}


void
Patch::deactivate()
{
	if (m_activated) {
	
		NodeBase::deactivate();
	
		for (List<Node*>::iterator i = m_nodes.begin(); i != m_nodes.end(); ++i) {
			if ((*i)->activated())
				(*i)->deactivate();
			assert(!(*i)->activated());
		}
	}
	assert(!m_activated);
}


void
Patch::process(bool b)
{
	if (!b) {
		// Write output buffers to 0
		for (List<InternalNode*>::iterator i = m_bridge_nodes.begin(); i != m_bridge_nodes.end(); ++i) {
			assert((*i)->as_port() != NULL);
			if ((*i)->as_port()->port_info()->is_output())
				(*i)->as_port()->clear_buffers();
		}
	}
	m_process = b;
}


/** Run the patch for the specified number of frames.
 * 
 * Calls all Nodes in the order m_process_order specifies.
 */
inline void
Patch::run(size_t nframes)
{
	if (m_process_order == NULL || !m_process)
		return;

	// Prepare all ports
	for (List<InternalNode*>::iterator i = m_bridge_nodes.begin(); i != m_bridge_nodes.end(); ++i)
		(*i)->as_port()->prepare_buffers(nframes);

	// Run all nodes
	for (size_t i=0; i < m_process_order->size(); ++i) {
		// Could be a gap due to a node removal event (see RemoveNodeEvent.cpp)
		// If you're thinking this isn't very nice, you're right.
		if (m_process_order->at(i) != NULL)
			m_process_order->at(i)->run(nframes);
	}
}


/** Returns the number of ports.
 *
 * Needs to override the NodeBase implementation since a Patch's ports are really
 * just it's input and output nodes' ports.
 */
size_t
Patch::num_ports() const
{
	return m_bridge_nodes.size();
}


#if 0
void
Patch::send_creation_messages(ClientInterface* client) const
{
	cerr << "FIXME: creation\n";

	om->client_broadcaster()->send_patch_to(client, this);
	
	for (List<Node*>::const_iterator j = m_nodes.begin(); j != m_nodes.end(); ++j) {
		Node* node = (*j);
		Port* port = node->as_port(); // NULL unless a bridge node
		node->send_creation_messages(client);
		
		usleep(100);

		// If this is a bridge (input/output) node, send the patch control value as well
		if (port != NULL && port->port_info()->is_control())
			om->client_broadcaster()->send_control_change_to(client, port->path(),
				((PortBase<sample>*)port)->buffer(0)->value_at(0));
	}
	
	for (List<Connection*>::const_iterator j = m_connections.begin(); j != m_connections.end(); ++j) {
		om->client_broadcaster()->send_connection_to(client, *j);
	}
	
	// Send port information
	/*for (size_t i=0; i < m_ports.size(); ++i) {
		Port* const port = m_ports.at(i);

		// Send metadata
		const map<string, string>& data = port->metadata();
		for (map<string, string>::const_iterator i = data.begin(); i != data.end(); ++i)
			om->client_broadcaster()->send_metadata_update_to(client, port->path(), (*i).first, (*i).second);
		
		if (port->port_info()->is_control())
			om->client_broadcaster()->send_control_change_to(client, port->path(),
				((PortBase<sample>*)port)->buffer(0)->value_at(0));
	}*/
}
#endif


void
Patch::add_to_store()
{
	// Add self and ports
	NodeBase::add_to_store();

	// Add nodes
	for (List<Node*>::iterator j = m_nodes.begin(); j != m_nodes.end(); ++j)
		(*j)->add_to_store();
}


void
Patch::remove_from_store()
{
	// Remove self and ports
	NodeBase::remove_from_store();

	// Remove nodes
	for (List<Node*>::iterator j = m_nodes.begin(); j != m_nodes.end(); ++j) {
		(*j)->remove_from_store();
		assert(om->object_store()->find((*j)->path()) == NULL);
	}
}


// Patch specific stuff


void
Patch::add_node(ListNode<Node*>* ln)
{
	assert(ln != NULL);
	assert(ln->elem() != NULL);
	assert(ln->elem()->parent_patch() == this);
	assert(ln->elem()->poly() == m_internal_poly || ln->elem()->poly() == 1);
	
	m_nodes.push_back(ln);
}


ListNode<Node*>*
Patch::remove_node(const string& name)
{
	for (List<Node*>::iterator i = m_nodes.begin(); i != m_nodes.end(); ++i)
		if ((*i)->name() == name)
			return m_nodes.remove(i);
	
	return NULL;
}


/** Remove a connection.  Realtime safe.
 */
ListNode<Connection*>*
Patch::remove_connection(const Port* src_port, const Port* dst_port)
{
	bool found = false;
	ListNode<Connection*>* connection = NULL;
	for (List<Connection*>::iterator i = m_connections.begin(); i != m_connections.end(); ++i) {
		if ((*i)->src_port() == src_port && (*i)->dst_port() == dst_port) {
			connection = m_connections.remove(i);
			found = true;
		}
	}

	if ( ! found)
		cerr << "WARNING:  [Patch::remove_connection] Connection not found !" << endl;

	return connection;
}


/** Remove a bridge_node.  Realtime safe.
 */
ListNode<InternalNode*>*
Patch::remove_bridge_node(const InternalNode* node)
{
	bool found = false;
	ListNode<InternalNode*>* bridge_node = NULL;
	for (List<InternalNode*>::iterator i = m_bridge_nodes.begin(); i != m_bridge_nodes.end(); ++i) {
		if ((*i) == node) {
			bridge_node = m_bridge_nodes.remove(i);
			found = true;
		}
	}

	if ( ! found)
		cerr << "WARNING:  [Patch::remove_bridge_node] InternalNode not found !" << endl;

	return bridge_node;
}


/** Find the process order for this Patch.
 *
 * The process order is a flat list that the patch will execute in order
 * when it's run() method is called.  Return value is a newly allocated list
 * which the caller is reponsible to delete.  Note that this function does
 * NOT actually set the process order, it is returned so it can be inserted
 * at the beginning of an audio cycle (by various Events).
 *
 * This function is not realtime safe, due to the use of the List<Node*> iterator
 */
Array<Node*>*
Patch::build_process_order() const
{
	Node*               node          = NULL;
	Array<Node*>* const process_order = new Array<Node*>(m_nodes.size());
	
	for (List<Node*>::const_iterator i = m_nodes.begin(); i != m_nodes.end(); ++i)
		(*i)->traversed(false);
		
	// Traverse backwards starting at outputs
	for (List<InternalNode*>::const_iterator i = m_bridge_nodes.begin(); i != m_bridge_nodes.end(); ++i) {
		node = (*i);
		assert(node->as_port() != NULL);
		
		// If the output node has been disconnected and has no connections left, don't traverse
		// into it so it's not in the process order (and can be removed w/o flaming segfault death)
		if (node->as_port()->port_info()->is_output() && node->providers()->size() > 0)
			build_process_order_recursive(node, process_order);
	}

	// Add any nodes that weren't hit by the traversal (disjoint nodes)
	for (List<Node*>::const_iterator i = m_nodes.begin(); i != m_nodes.end(); ++i) {
		node = (*i);
		if ( ! node->traversed()) {
			process_order->push_back(*i);
			node->traversed(true);
		}
	}

	assert(process_order->size() == m_nodes.size());

	return process_order;
}


/** Rename this Patch.
 *
 * This is responsible for updating the ObjectStore so the Patch can be
 * found at it's new path, as well as all it's children.
 */
void
Patch::set_path(const Path& new_path)
{
	const Path old_path = path();

	// Update nodes
	for (List<Node*>::iterator i = m_nodes.begin(); i != m_nodes.end(); ++i)
		(*i)->set_path(new_path.base_path() + (*i)->name());
	
	// Update self
	NodeBase::set_path(new_path);
}


} // namespace Om
