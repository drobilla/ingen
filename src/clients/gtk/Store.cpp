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
#include "GtkObjectController.h"
#include "PatchController.h"
#include "NodeController.h"
#include "PortController.h"
#include "PluginModel.h"
#include "SigClientInterface.h"

namespace OmGtk {

Store::Store(SigClientInterface& emitter)
{
	//emitter.new_plugin_sig.connect(sigc::mem_fun(this, &Store::add_plugin));
	emitter.object_destroyed_sig.connect(sigc::mem_fun(this, &Store::destruction_event));
	emitter.new_plugin_sig.connect(sigc::mem_fun(this, &Store::new_plugin_event));
}

void
Store::add_object(GtkObjectController* object)
{
	assert(object->path() != "");
	assert(m_objects.find(object->path()) == m_objects.end());

	m_objects[object->path()] = object;

	cout << "[Store] Added " << object->path() << endl;
}


void
Store::remove_object(GtkObjectController* object)
{
	if (!object)
		return;

	map<string, GtkObjectController*>::iterator i
		= m_objects.find(object->model()->path());

	if (i != m_objects.end()) {
		assert((*i).second == object);
		m_objects.erase(i);
	} else {
		cerr << "[App] Unable to find object " << object->model()->path()
			<< " to remove." << endl;
	}
	
	cout << "[Store] Removed " << object->path() << endl;
}


GtkObjectController*
Store::object(const string& path) const
{
	assert(path.length() > 0);
	map<string, GtkObjectController*>::const_iterator i = m_objects.find(path);
	if (i == m_objects.end())
		return NULL;
	else
		return (*i).second;
}


PatchController*
Store::patch(const string& path) const
{
	assert(path.length() > 0);
	map<string, GtkObjectController*>::const_iterator i = m_objects.find(path);
	if (i == m_objects.end())
		return NULL;
	else
		return dynamic_cast<PatchController*>((*i).second);
}


NodeController*
Store::node(const string& path) const
{
	assert(path.length() > 0);
	map<string, GtkObjectController*>::const_iterator i = m_objects.find(path);
	if (i == m_objects.end())
		return NULL;
	else
		return dynamic_cast<NodeController*>((*i).second);
}


PortController*
Store::port(const string& path) const
{
	assert(path.length() > 0);
	map<string, GtkObjectController*>::const_iterator i = m_objects.find(path);
	if (i == m_objects.end()) {
		return NULL;
	} else {
		// Normal port
		PortController* const pc = dynamic_cast<PortController*>((*i).second);
		if (pc != NULL)
			return pc;
		
		// Patch port (corresponding Node is in store)
		NodeController* const nc = dynamic_cast<NodeController*>((*i).second);
		if (nc != NULL)
			return nc->as_port(); // Patch port (maybe)
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


/* ****** Slots ******** */

void
Store::destruction_event(const string& path)
{
	remove_object(object(path));
}

void
Store::new_plugin_event(const string& type, const string& uri, const string& name)
{
	PluginModel* const p = new PluginModel(type, uri);
	p->name(name);
	add_plugin(p);
}

} // namespace OmGtk

