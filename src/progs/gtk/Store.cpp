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

#include "Store.h"
#include "ObjectModel.h"
#include "PatchModel.h"
#include "NodeModel.h"
#include "PortModel.h"
#include "PluginModel.h"
#include "PatchModel.h"
#include "SigClientInterface.h"

namespace OmGtk {

Store::Store(SigClientInterface& emitter)
{
	//emitter.new_plugin_sig.connect(sigc::mem_fun(this, &Store::add_plugin));
	emitter.object_destroyed_sig.connect(sigc::mem_fun(this, &Store::destruction_event));
	emitter.new_plugin_sig.connect(sigc::mem_fun(this, &Store::new_plugin_event));
	emitter.new_patch_sig.connect(sigc::mem_fun(this, &Store::new_patch_event));
	emitter.new_node_sig.connect(sigc::mem_fun(this, &Store::new_node_event));
	emitter.new_port_sig.connect(sigc::mem_fun(this, &Store::new_port_event));
}


void
Store::add_object(ObjectModel* object)
{
	assert(object->path() != "");
	assert(m_objects.find(object->path()) == m_objects.end());

	m_objects[object->path()] = object;

	cout << "[Store] Added " << object->path() << endl;
}


void
Store::remove_object(ObjectModel* object)
{
	if (!object)
		return;

	map<string, ObjectModel*>::iterator i
		= m_objects.find(object->path());

	if (i != m_objects.end()) {
		assert((*i).second == object);
		m_objects.erase(i);
	} else {
		cerr << "[App] Unable to find object " << object->path()
			<< " to remove." << endl;
	}
	
	cout << "[Store] Removed " << object->path() << endl;
}


CountedPtr<ObjectModel>
Store::object(const string& path) const
{
	assert(path.length() > 0);
	map<string, ObjectModel*>::const_iterator i = m_objects.find(path);
	if (i == m_objects.end())
		return NULL;
	else
		return (*i).second;
}


CountedPtr<PatchModel>
Store::patch(const string& path) const
{
	assert(path.length() > 0);
	map<string, ObjectModel*>::const_iterator i = m_objects.find(path);
	if (i == m_objects.end())
		return NULL;
	else
		return dynamic_cast<PatchModel*>((*i).second);
}


CountedPtr<NodeModel>
Store::node(const string& path) const
{
	assert(path.length() > 0);
	map<string, ObjectModel*>::const_iterator i = m_objects.find(path);
	if (i == m_objects.end())
		return NULL;
	else
		return dynamic_cast<NodeModel*>((*i).second);
}


CountedPtr<PortModel>
Store::port(const string& path) const
{
	assert(path.length() > 0);
	map<string, ObjectModel*>::const_iterator i = m_objects.find(path);
	if (i == m_objects.end()) {
		return NULL;
	} else {
		// Normal port
		PortModel* const pc = dynamic_cast<PortModel*>((*i).second);
		if (pc)
			return pc;
		
		// Patch port (corresponding Node is in store)
		// FIXME
		//
		/*
		NodeModel* const nc = dynamic_cast<NodeModel*>((*i).second);
		if (nc)
			return nc->as_port(); // Patch port (maybe)
		*/
	}

	return NULL;
}


void
Store::add_plugin(const PluginModel* pm)
{
	if (m_plugins.find(pm->uri()) != m_plugins.end()) {
		cerr << "DUPE! " << pm->uri() << endl;
		delete m_plugins[pm->uri()];
	}
	
	m_plugins[pm->uri()] = pm;
}



/* ****** Signal Handlers ******** */


void
Store::destruction_event(const string& path)
{
	remove_object(object(path).get());
}

void
Store::new_plugin_event(const string& type, const string& uri, const string& name)
{
	PluginModel* const p = new PluginModel(type, uri);
	p->name(name);
	add_plugin(p);
}


void
Store::new_patch_event(const string& path, uint32_t poly)
{
	PatchModel* const p = new PatchModel(path, poly);
	add_object(p);
}

void
Store::new_node_event(const string& plugin_type, const string& plugin_uri, const string& node_path, bool is_polyphonic, uint32_t num_ports)
{
	// FIXME: resolve plugin here
	
	NodeModel* const n = new NodeModel(node_path);
	n->polyphonic(is_polyphonic);
	// FIXME: num_ports unused
	add_object(n);
}


void
Store::new_port_event(const string& path, const string& type, bool is_output)
{
	// FIXME: this sucks
	
	PortModel::Type ptype = PortModel::CONTROL;
	if (type == "AUDIO") ptype = PortModel::AUDIO;
	else if (type == "CONTROL") ptype = PortModel::CONTROL;
	else if (type== "MIDI") ptype = PortModel::MIDI;
	else cerr << "[OSCListener] WARNING:  Unknown port type received (" << type << ")" << endl;
	
	PortModel::Direction pdir = is_output ? PortModel::OUTPUT : PortModel::INPUT;
	
	PortModel* const p = new PortModel(path, ptype, pdir);
	add_object(p);
	
}


} // namespace OmGtk

