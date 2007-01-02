/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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

#include "NodeBase.h"
#include <cassert>
#include <iostream>
#include <stdint.h>
#include "util.h"
#include "Array.h"
#include "Plugin.h"
#include "ClientBroadcaster.h"
#include "Port.h"
#include "List.h"
#include "Patch.h"
#include "ObjectStore.h"

using std::cout; using std::cerr; using std::endl;

namespace Ingen {


NodeBase::NodeBase(const Plugin* plugin, const string& name, size_t poly, Patch* parent, SampleRate srate, size_t buffer_size)
: Node(parent, name),
  _store(NULL),
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
	//_engine.broadcaster()->send_node_to(client, this);
}
*/

void
NodeBase::add_to_store(ObjectStore* store)
{
	assert(!_store);

	store->add(this);
	
	for (size_t i=0; i < num_ports(); ++i)
		store->add(_ports->at(i));

	_store = store;
}


void
NodeBase::remove_from_store()
{
	// Remove self
	TreeNode<GraphObject*>* node = _store->remove(path());
	if (node != NULL) {
		assert(_store->find(path()) == NULL);
		delete node;
	}
	
	// Remove ports
	for (size_t i=0; i < num_ports(); ++i) {
		node = _store->remove(_ports->at(i)->path());
		if (node != NULL) {
			assert(_store->find(_ports->at(i)->path()) == NULL);
			delete node;
		}
	}

	_store = NULL;
}


void
NodeBase::set_buffer_size(size_t size)
{
	_buffer_size = size;
	
	if (_ports)
		for (size_t i=0; i < _ports->size(); ++i)
			_ports->at(i)->set_buffer_size(size);
}


/** Runs the Node for the specified number of frames (block size)
 */
void
NodeBase::process(SampleCount nframes, FrameTime start, FrameTime end)
{
	assert(_activated);
	// Mix down any ports with multiple inputs
	for (size_t i=0; i < _ports->size(); ++i)
		_ports->at(i)->process(nframes, start, end);
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
		treenode = _store->remove(old_path +"/"+ _ports->at(i)->name());
		assert(treenode != NULL);
		assert(treenode->node() == _ports->at(i));
		treenode->key(new_path +"/" + _ports->at(i)->name());
		_store->add(treenode);
	}
	
	// Rename and reinsert self
	treenode = _store->remove(old_path);
	assert(treenode != NULL);
	assert(treenode->node() == this);
	GraphObject::set_path(new_path);
	treenode->key(new_path);
	_store->add(treenode);
	

	assert(_store->find(new_path) == this);
}


} // namespace Ingen

