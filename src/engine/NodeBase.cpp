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

#include "NodeBase.h"
#include <cassert>
#include <iostream>
#include <stdint.h>
#include "Om.h"
#include "OmApp.h"
#include "util.h"
#include "Array.h"
#include "Plugin.h"
#include "ClientBroadcaster.h"
#include "Port.h"
#include "List.h"
#include "Patch.h"
#include "ObjectStore.h"

using std::cout; using std::cerr; using std::endl;

namespace Om {


NodeBase::NodeBase(const string& name, size_t poly, Patch* parent, samplerate srate, size_t buffer_size)
: Node(parent, name),
  m_poly(poly),
  m_srate(srate),
  m_buffer_size(buffer_size),
  m_activated(false),
  m_num_ports(0),
  m_traversed(false),
  m_providers(new List<Node*>()),
  m_dependants(new List<Node*>())
{
	assert(m_poly > 0);
	assert(m_parent == NULL || (m_poly == parent->internal_poly() || m_poly == 1));
}


NodeBase::~NodeBase()
{
	assert(!m_activated);

	delete m_providers;
	delete m_dependants;
	
	for (size_t i=0; i < m_ports.size(); ++i)
		delete m_ports.at(i);
}


void
NodeBase::activate()
{
	assert(!m_activated);
	m_activated = true;
}


void
NodeBase::deactivate()
{
	assert(m_activated);
	m_activated = false;
}


/*
void
NodeBase::send_creation_messages(ClientInterface* client) const
{
	cerr << "FIXME: send_creation\n";
	//om->client_broadcaster()->send_node_to(client, this);
}
*/

void
NodeBase::add_to_store()
{
	om->object_store()->add(this);
	for (size_t i=0; i < num_ports(); ++i)
		om->object_store()->add(m_ports.at(i));
}


void
NodeBase::remove_from_store()
{
	// Remove self
	TreeNode<OmObject*>* node = om->object_store()->remove(path());
	if (node != NULL) {
		assert(om->object_store()->find(path()) == NULL);
		delete node;
	}
	
	// Remove ports
	for (size_t i=0; i < m_num_ports; ++i) {
		node = om->object_store()->remove(m_ports.at(i)->path());
		if (node != NULL) {
			assert(om->object_store()->find(m_ports.at(i)->path()) == NULL);
			delete node;
		}
	}
}


/** Runs the Node for the specified number of frames (block size)
 */
void
NodeBase::run(size_t nframes)
{
	assert(m_activated);
	// Mix down any ports with multiple inputs
	Port* p;
	for (size_t i=0; i < m_ports.size(); ++i) {
		p = m_ports.at(i);
		p->prepare_buffers(nframes);
	}
}


/** Rename this Node.
 *
 * This is responsible for updating the ObjectStore so the Node can be
 * found at it's new path, as well as all it's children.
 */
void
NodeBase::set_path(const Path& new_path)
{
	const Path old_path = path();
	//cerr << "Renaming " << old_path << " -> " << new_path << endl;
	
	TreeNode<OmObject*>* treenode = NULL;
	
	// Reinsert ports
	for (size_t i=0; i < m_num_ports; ++i) {
		treenode = om->object_store()->remove(old_path +"/"+ m_ports.at(i)->name());
		assert(treenode != NULL);
		assert(treenode->node() == m_ports.at(i));
		treenode->key(new_path +"/" + m_ports.at(i)->name());
		om->object_store()->add(treenode);
	}
	
	// Rename and reinsert self
	treenode = om->object_store()->remove(old_path);
	assert(treenode != NULL);
	assert(treenode->node() == this);
	OmObject::set_path(new_path);
	treenode->key(new_path);
	om->object_store()->add(treenode);
	

	assert(om->object_store()->find(new_path) == this);
}


} // namespace Om

