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

#include "BridgeNode.h"
//#include "ClientBroadcaster.h"
#include "Plugin.h"
#include "Patch.h"
#include "Om.h"
#include "OmApp.h"
#include "Maid.h"
#include "Driver.h"
#include "PortInfo.h"
#include <cassert>

namespace Om {

template <typename T>
BridgeNode<T>::BridgeNode(const string& path, size_t poly, Patch* parent, samplerate srate, size_t buffer_size)
: InternalNode(path,
	(parent->parent_patch() == NULL || poly != parent->parent_patch()->poly()) ? 1 : poly,
	//poly,
	parent, srate, buffer_size),
  m_driver_port(NULL),
  m_listnode(NULL),
  m_external_port(NULL)
{
	//cerr << "Creating bridge node " << path << " - polyphony: " << m_poly << endl;
	m_listnode = new ListNode<InternalNode*>(this);
}
template
BridgeNode<sample>::BridgeNode(const string& path, size_t poly, Patch* parent, samplerate srate, size_t buffer_size);
template
BridgeNode<MidiMessage>::BridgeNode(const string& path, size_t poly, Patch* parent, samplerate srate, size_t buffer_size);


template <typename T>
BridgeNode<T>::~BridgeNode()
{
	delete m_driver_port;
}
template BridgeNode<sample>::~BridgeNode();
template BridgeNode<MidiMessage>::~BridgeNode();


template <typename T>
void
BridgeNode<T>::activate()
{
	assert(om->template driver<T>() != NULL);
	assert(m_external_port != NULL); // Derived classes must create this
	assert(parent_patch() != NULL);
	
	if (parent_patch()->parent() == NULL && om != NULL)
		m_driver_port = om->template driver<T>()->create_port(m_external_port);

	InternalNode::activate();
}


template <typename T>
void
BridgeNode<T>::deactivate()
{
	if (m_is_added)
		remove_from_patch();
	
	InternalNode::deactivate();
	
	if (m_driver_port != NULL) {
		delete m_driver_port;
		m_driver_port = NULL;
	}
}

	
template <typename T>
void
BridgeNode<T>::add_to_patch()
{
	assert(parent_patch() != NULL);
	
	parent_patch()->add_bridge_node(m_listnode);

	InternalNode::add_to_patch();
	
	// Activate driver port now in the audio thread (not before when created, to avoid race issues)
	if (m_driver_port != NULL)
		m_driver_port->add_to_driver();
}


template <typename T>
void
BridgeNode<T>::remove_from_patch()
{
	assert(parent_patch() != NULL);
	
	if (m_is_added) {
		if (m_driver_port != NULL)
			m_driver_port->remove_from_driver();
		ListNode<InternalNode*>* ln = NULL;
		ln = parent_patch()->remove_bridge_node(this);
		
		om->maid()->push(ln);
		m_listnode = NULL;
		
	}
	InternalNode::remove_from_patch();
}


template <typename T>
void
BridgeNode<T>::set_path(const Path& new_path)
{
	InternalNode::set_path(new_path);
	
	m_external_port->set_path(new_path);

	if (m_driver_port != NULL)
		m_driver_port->set_name(path().c_str());
}


#if 0
template <typename T>
void
BridgeNode<T>::send_creation_messages(ClientInterface* client) const
{
	InternalNode::send_creation_messages(client);
	om->client_broadcaster()->send_new_port_to(client, m_external_port);
	
	// Send metadata
	for (map<string, string>::const_iterator i = metadata().begin(); i != metadata().end(); ++i)
		om->client_broadcaster()->send_metadata_update_to(client, path(), (*i).first, (*i).second);
	
	// Send control value (if necessary)
	//if (m_external_port->port_info()->is_control())
	//	om->client_broadcaster()->send_control_change_to(client, path(),
	//		((PortBase<sample>*)m_external_port)->buffer(0)->value_at(0));
}
#endif


} // namespace Om

