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

namespace LibOmClient {

Store::Store(SigClientInterface& emitter)
{
	//emitter.new_plugin_sig.connect(sigc::mem_fun(this, &Store::add_plugin));
	emitter.object_destroyed_sig.connect(sigc::mem_fun(this, &Store::destruction_event));
	emitter.new_plugin_sig.connect(sigc::mem_fun(this, &Store::new_plugin_event));
	emitter.new_patch_sig.connect(sigc::mem_fun(this, &Store::new_patch_event));
	emitter.new_node_sig.connect(sigc::mem_fun(this, &Store::new_node_event));
	emitter.new_port_sig.connect(sigc::mem_fun(this, &Store::new_port_event));
	emitter.connection_sig.connect(sigc::mem_fun(this, &Store::connection_event));
	emitter.disconnection_sig.connect(sigc::mem_fun(this, &Store::disconnection_event));
	emitter.metadata_update_sig.connect(sigc::mem_fun(this, &Store::metadata_update_event));
}


void
Store::add_object(CountedPtr<ObjectModel> object)
{
	assert(object->path() != "");
	assert(m_objects.find(object->path()) == m_objects.end());

	m_objects[object->path()] = object;

	cout << "[Store] Added " << object->path() << endl;
}


CountedPtr<ObjectModel>
Store::remove_object(const string& path)
{
	map<string, CountedPtr<ObjectModel> >::iterator i = m_objects.find(path);

	if (i != m_objects.end()) {
		assert((*i).second->path() == path);
		CountedPtr<ObjectModel> result = (*i).second;
		m_objects.erase(i);
		cout << "[Store] Removed " << path << endl;
		return result;
	} else {
		cerr << "[Store] Unable to find object " << path << " to remove." << endl;
		return NULL;
	}
}


CountedPtr<PluginModel>
Store::plugin(const string& uri)
{
	assert(uri.length() > 0);
	map<string, CountedPtr<PluginModel> >::iterator i = m_plugins.find(uri);
	if (i == m_plugins.end())
		return NULL;
	else
		return (*i).second;
}


CountedPtr<ObjectModel>
Store::object(const string& path)
{
	assert(path.length() > 0);
	map<string, CountedPtr<ObjectModel> >::iterator i = m_objects.find(path);
	if (i == m_objects.end())
		return NULL;
	else
		return (*i).second;
}


CountedPtr<PatchModel>
Store::patch(const string& path)
{
	assert(path.length() > 0);
	map<string, CountedPtr<ObjectModel> >::iterator i = m_objects.find(path);
	if (i == m_objects.end())
		return NULL;
	else
		return (CountedPtr<PatchModel>)(*i).second; // FIXME
}


CountedPtr<NodeModel>
Store::node(const string& path)
{
	assert(path.length() > 0);
	map<string, CountedPtr<ObjectModel> >::iterator i = m_objects.find(path);
	if (i == m_objects.end())
		return NULL;
	else
		return (*i).second;
}


CountedPtr<PortModel>
Store::port(const string& path)
{
	assert(path.length() > 0);
	map<string, CountedPtr<ObjectModel> >::iterator i = m_objects.find(path);
	if (i == m_objects.end()) {
		return NULL;
	} else {
		// Normal port
		/*PortModel* const pc = dynamic_cast<PortModel*>((*i).second);
		if (pc)
			return pc;*/
		return (*i).second;
		
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
Store::add_plugin(CountedPtr<PluginModel> pm)
{
	if (m_plugins.find(pm->uri()) != m_plugins.end()) {
		cerr << "DUPE PLUGIN: " << pm->uri() << endl;
	} else {
		m_plugins[pm->uri()] = pm;
	}
}



/* ****** Signal Handlers ******** */


void
Store::destruction_event(const string& path)
{
	// I'm assuming the compiler will optimize out all these const
	// pointers into one...
	
	CountedPtr<ObjectModel> obj_ptr = remove_object(path);
	ObjectModel* const object = obj_ptr.get();

	// FIXME: Why does this need to be specific?  Just make a remove_child
	// for everything
	
	// Succeeds for (Plugin) Nodes and Patches
	NodeModel* const node = dynamic_cast<NodeModel*>(object);
	if (node) {
		cerr << "Node\n";
		PatchModel* const parent = dynamic_cast<PatchModel* const>(object->parent().get());
		if (parent)
			parent->remove_node(node->name());
	}

	PortModel* const port = dynamic_cast<PortModel*>(object);
	if (port) {
		NodeModel* const parent = dynamic_cast<NodeModel* const>(object->parent().get());
		assert(parent);
		parent->remove_port(port->name());
	}

	// FIXME: emit signals
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
	// FIXME: What to do with a conflict?
	
	if (m_objects.find(path) == m_objects.end()) {
		CountedPtr<PatchModel> p(new PatchModel(path, poly));
		add_object(p);
		
		CountedPtr<PatchModel> parent = object(p->path().parent());
		if (parent) {
			p->set_parent(parent);
			parent->add_node(p);
			assert(p->parent() == parent);
		} else {
			cerr << "ERROR: new patch with no parent" << endl;
		}
	}
}


void
Store::new_node_event(const string& plugin_type, const string& plugin_uri, const string& node_path, bool is_polyphonic, uint32_t num_ports)
{
	// FIXME: What to do with a conflict?

	if (m_objects.find(node_path) == m_objects.end()) {

		CountedPtr<PluginModel> plug = plugin(plugin_uri);
		assert(plug);

		CountedPtr<NodeModel> n(new NodeModel(plug, node_path));
		n->polyphonic(is_polyphonic);
		// FIXME: num_ports unused
		add_object(n);

		//std::map<string, CountedPtr<ObjectModel> >::iterator pi = m_objects.find(n->path().parent());
		//if (pi != m_objects.end()) {
		CountedPtr<PatchModel> parent = object(n->path().parent());
		if (parent) {
			n->set_parent(parent);
			assert(n->parent() == parent);
			parent->add_node(n);
			assert(n->parent() == parent);
		} else {
			cerr << "ERROR: new node with no parent" << endl;
		}
	}
}


void
Store::new_port_event(const string& path, const string& type, bool is_output)
{
	// FIXME: this sucks
	/*
	if (m_objects.find(path) == m_objects.end()) {
		PortModel::Type ptype = PortModel::CONTROL;
		if (type == "AUDIO") ptype = PortModel::AUDIO;
		else if (type == "CONTROL") ptype = PortModel::CONTROL;
		else if (type== "MIDI") ptype = PortModel::MIDI;
		else cerr << "[OSCListener] WARNING:  Unknown port type received (" << type << ")" << endl;
		
		PortModel::Direction pdir = is_output ? PortModel::OUTPUT : PortModel::INPUT;
		
		PortModel* const p = new PortModel(path, ptype, pdir);

		add_object(p);
		} else
		*/
	if (m_objects.find(path) == m_objects.end()) {
		
		PortModel::Type ptype = PortModel::CONTROL;
		if (type == "AUDIO") ptype = PortModel::AUDIO;
		else if (type == "CONTROL") ptype = PortModel::CONTROL;
		else if (type== "MIDI") ptype = PortModel::MIDI;
		else cerr << "[OSCListener] WARNING:  Unknown port type received (" << type << ")" << endl;
		
		PortModel::Direction pdir = is_output ? PortModel::OUTPUT : PortModel::INPUT;

		CountedPtr<PortModel> p(new PortModel(path, ptype, pdir));
		add_object(p);

		std::map<string, CountedPtr<ObjectModel> >::iterator pi = m_objects.find(p->path().parent());
		if (pi != m_objects.end()) {
			CountedPtr<NodeModel> parent = (*pi).second;
			p->set_parent(parent);
			if (parent) {
				parent->add_port(p);
				assert(p->parent() == parent);
			} else {
				cerr << "ERROR: new port with no parent" << endl;
			}
		}
	}
}


void
Store::metadata_update_event(const string& subject_path, const string& predicate, const string& value)
{
	CountedPtr<ObjectModel> subject = object(subject_path);
	if (subject)
		subject->set_metadata(predicate, value);
	else
		cerr << "ERROR: metadata for nonexistant object." << endl;
}


void
Store::connection_event(const string& src_port_path, const string& dst_port_path)
{
	const Path& src = src_port_path;
	const Path& dst = dst_port_path;

	assert(src.parent().parent() == dst.parent().parent());
	const Path& patch_path = src.parent().parent();

	CountedPtr<PatchModel> patch = this->patch(patch_path);
	if (patch)
		patch->add_connection(new ConnectionModel(src, dst));
	else
		cerr << "ERROR: connection in nonexistant patch" << endl;
}


void
Store::disconnection_event(const string& src_port_path, const string& dst_port_path)
{
	const Path& src = src_port_path;
	const Path& dst = dst_port_path;

	assert(src.parent().parent() == dst.parent().parent());
	const Path& patch_path = src.parent().parent();

	CountedPtr<PatchModel> patch = this->patch(patch_path);
	if (patch)
		patch->remove_connection(src, dst);
	else
		cerr << "ERROR: connection in nonexistant patch" << endl;
}


} // namespace LibOmClient

