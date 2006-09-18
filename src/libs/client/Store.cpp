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



Store::Store(CountedPtr<EngineInterface> engine, CountedPtr<SigClientInterface> emitter)
: _engine(engine)
, _emitter(emitter)
{
	emitter->object_destroyed_sig.connect(sigc::mem_fun(this, &Store::destruction_event));
	emitter->new_plugin_sig.connect(sigc::mem_fun(this, &Store::new_plugin_event));
	emitter->new_patch_sig.connect(sigc::mem_fun(this, &Store::new_patch_event));
	emitter->new_node_sig.connect(sigc::mem_fun(this, &Store::new_node_event));
	emitter->new_port_sig.connect(sigc::mem_fun(this, &Store::new_port_event));
	emitter->patch_enabled_sig.connect(sigc::mem_fun(this, &Store::patch_enabled_event));
	emitter->patch_disabled_sig.connect(sigc::mem_fun(this, &Store::patch_disabled_event));
	emitter->patch_cleared_sig.connect(sigc::mem_fun(this, &Store::patch_cleared_event));
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

	_engine->request_plugin(node->plugin_uri());

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
	map<string, list<CountedPtr<NodeModel> > >::iterator n
		= m_plugin_orphans.find(plugin->uri());

	if (n != m_plugin_orphans.end()) {
	
		list<CountedPtr<NodeModel> > spawn = n->second; // take a copy

		m_plugin_orphans.erase(plugin->uri()); // prevent infinite recursion
		
		for (list<CountedPtr<NodeModel> >::iterator i = spawn.begin();
				i != spawn.end(); ++i) {
			add_object(*i);
		}
	}
}


void
Store::add_connection_orphan(CountedPtr<ConnectionModel> connection)
{
	cerr << "WARNING: Orphan connection " << connection->src_port_path()
		<< " -> " << connection->dst_port_path() << " received." << endl;
	
	m_connection_orphans.push_back(connection);
}


void
Store::resolve_connection_orphans(CountedPtr<PortModel> port)
{
	assert(port->parent());

	for (list<CountedPtr<ConnectionModel> >::iterator c = m_connection_orphans.begin();
			c != m_connection_orphans.end(); ) {
		
		if ((*c)->src_port_path() == port->path())
			(*c)->set_src_port(port);
		
		if ((*c)->dst_port_path() == port->path())
			(*c)->set_dst_port(port);

		list<CountedPtr<ConnectionModel> >::iterator next = c;
		++next;
		
		if ((*c)->src_port() && (*c)->dst_port()) {
			CountedPtr<PatchModel> patch = PtrCast<PatchModel>(this->object((*c)->patch_path()));
			if (patch) {
				cerr << "Resolved orphan connection " << (*c)->src_port_path() <<
					(*c)->dst_port_path() << endl;
				patch->add_connection(*c);
				m_connection_orphans.erase(c);
			}
		}

		c = next;
	}
}


void
Store::add_orphan(CountedPtr<ObjectModel> child)
{
	cerr << "WARNING: Orphan object " << child->path() << " received." << endl;

	map<Path, list<CountedPtr<ObjectModel> > >::iterator children
		= m_orphans.find(child->path().parent());

	_engine->request_object(child->path().parent());

	if (children != m_orphans.end()) {
		children->second.push_back(child);
	} else {
		list<CountedPtr<ObjectModel> > l;
		l.push_back(child);
		m_orphans[child->path().parent()] = l;
	}
}


void
Store::add_metadata_orphan(const Path& subject_path, const string& predicate, const Atom& value)
{
	map<Path, list<std::pair<string, Atom> > >::iterator orphans
		= m_metadata_orphans.find(subject_path);

	_engine->request_object(subject_path);

	if (orphans != m_metadata_orphans.end()) {
		orphans->second.push_back(std::pair<string, Atom>(predicate, value));
	} else {
		list<std::pair<string, Atom> > l;
		l.push_back(std::pair<string, Atom>(predicate, value));
		m_metadata_orphans[subject_path] = l;
	}
}


void
Store::resolve_metadata_orphans(CountedPtr<ObjectModel> subject)
{
	map<Path, list<std::pair<string, Atom> > >::iterator v
		= m_metadata_orphans.find(subject->path());

	if (v != m_metadata_orphans.end()) {
	
		list<std::pair<string, Atom> > values = v->second; // take a copy

		m_metadata_orphans.erase(subject->path());
		
		for (list<std::pair<string, Atom> >::iterator i = values.begin();
				i != values.end(); ++i) {
			subject->set_metadata(i->first, i->second);
		}
	}
}


void
Store::resolve_orphans(CountedPtr<ObjectModel> parent)
{
	map<Path, list<CountedPtr<ObjectModel> > >::iterator c
		= m_orphans.find(parent->path());

	if (c != m_orphans.end()) {
	
		list<CountedPtr<ObjectModel> > children = c->second; // take a copy

		m_orphans.erase(parent->path()); // prevent infinite recursion
		
		for (list<CountedPtr<ObjectModel> >::iterator i = children.begin();
				i != children.end(); ++i) {
			add_object(*i);
		}
	}
}


void
Store::add_object(CountedPtr<ObjectModel> object)
{
	// If we already have "this" object, merge the existing one into the new
	// one (with precedence to the new values).
	ObjectMap::iterator existing = m_objects.find(object->path());
	if (existing != m_objects.end()) {
		cerr << "[Store] Warning:  Assimilating " << object->path() << endl;
		object->assimilate(existing->second);
		existing->second = object;
	} else {

		if (object->path() != "/") {
			CountedPtr<ObjectModel> parent = this->object(object->path().parent());
			if (parent) {
				assert(object->path().is_child_of(parent->path()));
				object->set_parent(parent);
				parent->add_child(object);
				assert(parent && (object->parent() == parent));
				
				m_objects[object->path()] = object;
				new_object_sig.emit(object);
				
				resolve_metadata_orphans(parent);
				resolve_orphans(parent);

			} else {
				add_orphan(object);
			}
		} else {
			m_objects[object->path()] = object;
			new_object_sig.emit(object);
		}

	}

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

		if (result)
			result->destroyed_sig.emit();

		if (result->path() != "/") {
			assert(result->parent());

			CountedPtr<ObjectModel> parent = this->object(result->path().parent());
			if (parent) {
				parent->remove_child(result);
			}
		}
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
	// FIXME: dupes?  assimilate?
	
	m_plugins[pm->uri()] = pm;
}



/* ****** Signal Handlers ******** */


void
Store::destruction_event(const Path& path)
{
	CountedPtr<ObjectModel> removed = remove_object(path);

	removed.reset();

	//cerr << "Store removed object " << path
	//	<< ", count: " << removed.use_count();
}

void
Store::new_plugin_event(const string& uri, const string& name)
{
	CountedPtr<PluginModel> p(new PluginModel(uri, name));
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
Store::new_node_event(const string& plugin_uri, const Path& node_path, bool is_polyphonic, uint32_t num_ports)
{
	// FIXME: num_ports unused
	
	CountedPtr<PluginModel> plug = plugin(plugin_uri);
	if (!plug) {
		CountedPtr<NodeModel> n(new NodeModel(plugin_uri, node_path, is_polyphonic));
		add_plugin_orphan(n);
	} else {
		CountedPtr<NodeModel> n(new NodeModel(plug, node_path, is_polyphonic));
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
	if (p->parent())
		resolve_connection_orphans(p);
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
Store::patch_cleared_event(const Path& path)
{
	CountedPtr<PatchModel> patch = PtrCast<PatchModel>(object(path));
	if (patch) {
		NodeModelMap children = patch->nodes(); // take a copy
		for (NodeModelMap::iterator i = children.begin(); i != children.end(); ++i) {
			destruction_event(i->second->path());
		}
	}
}


void
Store::metadata_update_event(const Path& subject_path, const string& predicate, const Atom& value)
{
	CountedPtr<ObjectModel> subject = object(subject_path);
	
	if (subject) {
		subject->set_metadata(predicate, value);
	} else {
		add_metadata_orphan(subject_path, predicate, value);
		cerr << "WARNING: metadata for unknown object." << endl;
	}
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

	CountedPtr<ConnectionModel> dangling_cm(new ConnectionModel(src_port_path, dst_port_path));

	if (src_port && src_port->parent() && dst_port && dst_port->parent()) { 
	
		CountedPtr<PatchModel> patch = PtrCast<PatchModel>(this->object(dangling_cm->patch_path()));
		assert(patch);

		CountedPtr<ConnectionModel> cm(new ConnectionModel(src_port, dst_port));
		
		src_port->connected_to(dst_port);
		dst_port->connected_to(src_port);

		patch->add_connection(cm);
	
	} else {

		add_connection_orphan(dangling_cm);

	}
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

