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


NodeBase::NodeBase(const Plugin* plugin, const string& name, size_t poly, Patch* parent, samplerate srate, size_t buffer_size)
: Node(parent, name),
  _plugin(plugin),
  _poly(poly),
  _srate(srate),
  _buffer_size(buffer_size),
  _activated(false),
  _ports(NULL),
  _traversed(false),
  _providers(new List<Node*>()),
  _dependants(new List<Node*>())
{
	assert(_plugin);
	assert(_poly > 0);
	assert(_parent == NULL || (_poly == parent->internal_poly() || _poly == 1));
}


NodeBase::~NodeBase()
{
	assert(!_activated);

	delete _providers;
	delete _dependants;
	
	for (size_t i=0; i < num_ports(); ++i)
		delete _ports->at(i);
}


void
NodeBase::activate()
{
	assert(!_activated);
	_activated = true;
}


void
NodeBase::deactivate()
{
	assert(_activated);
	_activated = false;
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
		om->object_store()->add(_ports->at(i));
}


void
NodeBase::remove_from_store()
{
	// Remove self
	TreeNode<GraphObject*>* node = om->object_store()->remove(path());
	if (node != NULL) {
		assert(om->object_store()->find(path()) == NULL);
		delete node;
	}
	
	// Remove ports
	for (size_t i=0; i < num_ports(); ++i) {
		node = om->object_store()->remove(_ports->at(i)->path());
		if (node != NULL) {
			assert(om->object_store()->find(_ports->at(i)->path()) == NULL);
			delete node;
		}
	}
}


/** Runs the Node for the specified number of frames (block size)
 */
void
NodeBase::process(samplecount nframes)
{
	assert(_activated);
	// Mix down any ports with multiple inputs
	for (size_t i=0; i < _ports->size(); ++i)
		_ports->at(i)->process(nframes);
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
	
	TreeNode<GraphObject*>* treenode = NULL;
	
	// Reinsert ports
	for (size_t i=0; i < num_ports(); ++i) {
		treenode = om->object_store()->remove(old_path +"/"+ _ports->at(i)->name());
		assert(treenode != NULL);
		assert(treenode->node() == _ports->at(i));
		treenode->key(new_path +"/" + _ports->at(i)->name());
		om->object_store()->add(treenode);
	}
	
	// Rename and reinsert self
	treenode = om->object_store()->remove(old_path);
	assert(treenode != NULL);
	assert(treenode->node() == this);
	GraphObject::set_path(new_path);
	treenode->key(new_path);
	om->object_store()->add(treenode);
	

	assert(om->object_store()->find(new_path) == this);
}


} // namespace Om

