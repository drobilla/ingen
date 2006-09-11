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

#include "Store.h"
#include "ObjectModel.h"
#include "PatchModel.h"
#include "NodeModel.h"
#include "PortModel.h"
#include "PluginModel.h"
#include "PatchModel.h"
#include "SigClientInterface.h"

namespace Ingen {
namespace Client {



Store::Store(CountedPtr<SigClientInterface> emitter)
{
	//emitter.new_plugin_sig.connect(sigc::mem_fun(this, &Store::add_plugin));
	emitter->object_destroyed_sig.connect(sigc::mem_fun(this, &Store::destruction_event));
	emitter->new_plugin_sig.connect(sigc::mem_fun(this, &Store::new_plugin_event));
	emitter->new_patch_sig.connect(sigc::mem_fun(this, &Store::new_patch_event));
	emitter->new_node_sig.connect(sigc::mem_fun(this, &Store::new_node_event));
	emitter->new_port_sig.connect(sigc::mem_fun(this, &Store::new_port_event));
	emitter->patch_enabled_sig.connect(sigc::mem_fun(this, &Store::patch_enabled_event));
	emitter->patch_disabled_sig.connect(sigc::mem_fun(this, &Store::patch_disabled_event));
	emitter->connection_sig.connect(sigc::mem_fun(this, &Store::connection_event));
	emitter->disconnection_sig.connect(sigc::mem_fun(this, &Store::disconnection_event));
	emitter->metadata_update_sig.connect(sigc::mem_fun(this, &Store::metadata_update_event));
	emitter->control_change_sig.connect(sigc::mem_fun(this, &Store::control_change_event));
}


void
Store::clear()
{
	m_objects.clear();
	m_plugins.clear();

}


void
Store::add_plugin_orphan(CountedPtr<NodeModel> node)
{
	cerr << "WARNING: Node " << node->path() << " received, but plugin "
		<< node->plugin_uri() << " unknown." << endl;

	map<string, list<CountedPtr<NodeModel> > >::iterator spawn
		= m_plugin_orphans.find(node->plugin_uri());

	if (spawn != m_plugin_orphans.end()) {
		spawn->second.push_back(node);
	} else {
		list<CountedPtr<NodeModel> > l;
		l.push_back(node);
		m_plugin_orphans[node->plugin_uri()] = l;
	}
}


void
Store::resolve_plugin_orphans(CountedPtr<PluginModel> plugin)
{
	map<string, list<CountedPtr<NodeModel> > >::iterator spawn
		= m_plugin_orphans.find(plugin->uri());

	if (spawn != m_plugin_orphans.end()) {
		cerr << "XXXXXXXXXX PLUGIN-ORPHAN PLUGIN FOUND!! XXXXXXXXXXXXXXXXX" << endl;
	}
}


void
Store::add_orphan(CountedPtr<ObjectModel> child)
{
	cerr << "WARNING: Orphan object " << child->path() << " received." << endl;

	map<Path, list<CountedPtr<ObjectModel> > >::iterator children
		= m_orphans.find(child->path().parent());

	if (children != m_orphans.end()) {
		children->second.push_back(child);
	} else {
		list<CountedPtr<ObjectModel> > l;
		l.push_back(child);
		m_orphans[child->path().parent()] = l;
	}
}


void
Store::resolve_orphans(CountedPtr<ObjectModel> parent)
{
	map<Path, list<CountedPtr<ObjectModel> > >::iterator children
		= m_orphans.find(parent->path());

	if (children != m_orphans.end()) {
		cerr << "XXXXXXXXXXXXX ORPHAN PARENT FOUND!! XXXXXXXXXXXXXXXXX" << endl;
	}
}


void
Store::add_object(CountedPtr<ObjectModel> object)
{
	assert(object->path() != "");
	assert(m_objects.find(object->path()) == m_objects.end());

	if (object->path() != "/") {
		CountedPtr<ObjectModel> parent = this->object(object->path().parent());
		if (parent) {
			assert(object->path().is_child_of(parent->path()));
			object->set_parent(parent);
			parent->add_child(object);
			assert(object->parent() == parent);
		} else {
			add_orphan(object);
		}
	}

	m_objects[object->path()] = object;

	new_object_sig.emit(object);

	resolve_orphans(object);

	//cout << "[Store] Added " << object->path() << endl;
}


CountedPtr<ObjectModel>
Store::remove_object(const Path& path)
{
	map<Path, CountedPtr<ObjectModel> >::iterator i = m_objects.find(path);

	if (i != m_objects.end()) {
		assert((*i).second->path() == path);
		CountedPtr<ObjectModel> result = (*i).second;
		m_objects.erase(i);
		//cout << "[Store] Removed " << path << endl;
		return result;
	} else {
		cerr << "[Store] Unable to find object " << path << " to remove." << endl;
		return CountedPtr<ObjectModel>();
	}
}


CountedPtr<PluginModel>
Store::plugin(const string& uri)
{
	assert(uri.length() > 0);
	map<string, CountedPtr<PluginModel> >::iterator i = m_plugins.find(uri);
	if (i == m_plugins.end())
		return CountedPtr<PluginModel>();
	else
		return (*i).second;
}


CountedPtr<ObjectModel>
Store::object(const Path& path)
{
	assert(path.length() > 0);
	map<Path, CountedPtr<ObjectModel> >::iterator i = m_objects.find(path);
	if (i == m_objects.end())
		return CountedPtr<ObjectModel>();
	else
		return (*i).second;
}

void
Store::add_plugin(CountedPtr<PluginModel> pm)
{
	// FIXME: dupes?
	
	m_plugins[pm->uri()] = pm;
}



/* ****** Signal Handlers ******** */


void
Store::destruction_event(const Path& path)
{
	// Hopefully the compiler will optimize all these const pointers into one...
	
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
			parent->remove_node(node->path().name());
	}

	PortModel* const port = dynamic_cast<PortModel*>(object);
	if (port) {
		NodeModel* const parent = dynamic_cast<NodeModel* const>(object->parent().get());
		assert(parent);
		parent->remove_port(port->path().name());
	}

	if (object)
		object->destroyed_sig.emit();
}

void
Store::new_plugin_event(const string& type, const string& uri, const string& name)
{
	CountedPtr<PluginModel> p(new PluginModel(type, uri));
	p->name(name);
	add_plugin(p);
	resolve_plugin_orphans(p);
}


void
Store::new_patch_event(const Path& path, uint32_t poly)
{
	CountedPtr<PatchModel> p(new PatchModel(path, poly));
	add_object(p);
}


void
Store::new_node_event(const string& plugin_type, const string& plugin_uri, const Path& node_path, bool is_polyphonic, uint32_t num_ports)
{
	// FIXME: num_ports unused
	
	CountedPtr<PluginModel> plug = plugin(plugin_uri);
	if (!plug) {
		CountedPtr<NodeModel> n(new NodeModel(plugin_uri, node_path));
		n->polyphonic(is_polyphonic);
		add_plugin_orphan(n);
	} else {
		CountedPtr<NodeModel> n(new NodeModel(plug, node_path));
		n->polyphonic(is_polyphonic);
		add_object(n);
	}
}


void
Store::new_port_event(const Path& path, const string& type, bool is_output)
{
	// FIXME: this sucks

	PortModel::Type ptype = PortModel::CONTROL;
	if (type == "AUDIO") ptype = PortModel::AUDIO;
	else if (type == "CONTROL") ptype = PortModel::CONTROL;
	else if (type== "MIDI") ptype = PortModel::MIDI;
	else cerr << "[Store] WARNING:  Unknown port type received (" << type << ")" << endl;

	PortModel::Direction pdir = is_output ? PortModel::OUTPUT : PortModel::INPUT;

	CountedPtr<PortModel> p(new PortModel(path, ptype, pdir));
	add_object(p);
}


void
Store::patch_enabled_event(const Path& path)
{
	CountedPtr<PatchModel> patch = PtrCast<PatchModel>(object(path));
	if (patch)
		patch->enable();
}


void
Store::patch_disabled_event(const Path& path)
{
	CountedPtr<PatchModel> patch = PtrCast<PatchModel>(object(path));
	if (patch)
		patch->disable();
}


void
Store::metadata_update_event(const Path& subject_path, const string& predicate, const string& value)
{
	CountedPtr<ObjectModel> subject = object(subject_path);
	if (subject)
		subject->set_metadata(predicate, value);
	else
		cerr << "ERROR: metadata for nonexistant object." << endl;
}


void
Store::control_change_event(const Path& port_path, float value)
{
	CountedPtr<PortModel> port = PtrCast<PortModel>(object(port_path));
	if (port)
		port->value(value);
	else
		cerr << "ERROR: metadata for nonexistant object." << endl;
}


void
Store::connection_event(const Path& src_port_path, const Path& dst_port_path)
{
	CountedPtr<PortModel> src_port = PtrCast<PortModel>(object(src_port_path));
	CountedPtr<PortModel> dst_port = PtrCast<PortModel>(object(dst_port_path));

	assert(src_port);
	assert(dst_port);

	src_port->connected_to(dst_port);
	dst_port->connected_to(src_port);

	CountedPtr<ConnectionModel> cm(new ConnectionModel(src_port, dst_port));

	CountedPtr<PatchModel> patch = PtrCast<PatchModel>(this->object(cm->patch_path()));
	
	if (patch)
		patch->add_connection(cm);
	else
		cerr << "ERROR: connection in nonexistant patch" << endl;
}


void
Store::disconnection_event(const Path& src_port_path, const Path& dst_port_path)
{
	// Find the ports and create a ConnectionModel just to get at the parent path
	// finding logic in ConnectionModel.  So I'm lazy.
	
	CountedPtr<PortModel> src_port = PtrCast<PortModel>(object(src_port_path));
	CountedPtr<PortModel> dst_port = PtrCast<PortModel>(object(dst_port_path));

	assert(src_port);
	assert(dst_port);
	
	src_port->disconnected_from(dst_port);
	dst_port->disconnected_from(src_port);

	CountedPtr<ConnectionModel> cm(new ConnectionModel(src_port, dst_port));

	CountedPtr<PatchModel> patch = PtrCast<PatchModel>(this->object(cm->patch_path()));
	
	if (patch)
		patch->remove_connection(src_port_path, dst_port_path);
	else
		cerr << "ERROR: disconnection in nonexistant patch" << endl;
}


} // namespace Client
} // namespace Ingen

