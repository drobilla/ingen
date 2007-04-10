/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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



Store::Store(SharedPtr<EngineInterface> engine, SharedPtr<SigClientInterface> emitter)
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
	_objects.clear();
	_plugins.clear();

}


void
Store::add_plugin_orphan(SharedPtr<NodeModel> node)
{
	cerr << "WARNING: Node " << node->path() << " received, but plugin "
		<< node->plugin_uri() << " unknown." << endl;

	map<string, list<SharedPtr<NodeModel> > >::iterator spawn
		= _plugin_orphans.find(node->plugin_uri());

	_engine->request_plugin(node->plugin_uri());

	if (spawn != _plugin_orphans.end()) {
		spawn->second.push_back(node);
	} else {
		list<SharedPtr<NodeModel> > l;
		l.push_back(node);
		_plugin_orphans[node->plugin_uri()] = l;
	}
}


void
Store::resolve_plugin_orphans(SharedPtr<PluginModel> plugin)
{
	map<string, list<SharedPtr<NodeModel> > >::iterator n
		= _plugin_orphans.find(plugin->uri());

	if (n != _plugin_orphans.end()) {
	
		list<SharedPtr<NodeModel> > spawn = n->second; // take a copy

		_plugin_orphans.erase(plugin->uri()); // prevent infinite recursion
		
		for (list<SharedPtr<NodeModel> >::iterator i = spawn.begin();
				i != spawn.end(); ++i) {
			add_object(*i);
		}
	}
}


void
Store::add_connection_orphan(SharedPtr<ConnectionModel> connection)
{
	cerr << "WARNING: Orphan connection " << connection->src_port_path()
		<< " -> " << connection->dst_port_path() << " received." << endl;
	
	_connection_orphans.push_back(connection);
}


void
Store::resolve_connection_orphans(SharedPtr<PortModel> port)
{
	assert(port->parent());

	for (list<SharedPtr<ConnectionModel> >::iterator c = _connection_orphans.begin();
			c != _connection_orphans.end(); ) {
		
		if ((*c)->src_port_path() == port->path())
			(*c)->set_src_port(port);
		
		if ((*c)->dst_port_path() == port->path())
			(*c)->set_dst_port(port);

		list<SharedPtr<ConnectionModel> >::iterator next = c;
		++next;
		
		if ((*c)->src_port() && (*c)->dst_port()) {
			SharedPtr<PatchModel> patch = connection_patch((*c)->src_port_path(), (*c)->dst_port_path());
			if (patch) {
				cerr << "Resolved orphan connection " << (*c)->src_port_path() <<
					(*c)->dst_port_path() << endl;
				patch->add_connection(*c);
				_connection_orphans.erase(c);
			}
		}

		c = next;
	}
}


void
Store::add_orphan(SharedPtr<ObjectModel> child)
{
	cerr << "WARNING: Orphan object " << child->path() << " received." << endl;

	map<Path, list<SharedPtr<ObjectModel> > >::iterator children
		= _orphans.find(child->path().parent());

	_engine->request_object(child->path().parent());

	if (children != _orphans.end()) {
		children->second.push_back(child);
	} else {
		list<SharedPtr<ObjectModel> > l;
		l.push_back(child);
		_orphans[child->path().parent()] = l;
	}
}


void
Store::add_metadata_orphan(const Path& subject_path, const string& predicate, const Atom& value)
{
	map<Path, list<std::pair<string, Atom> > >::iterator orphans
		= _metadata_orphans.find(subject_path);

	_engine->request_object(subject_path);

	if (orphans != _metadata_orphans.end()) {
		orphans->second.push_back(std::pair<string, Atom>(predicate, value));
	} else {
		list<std::pair<string, Atom> > l;
		l.push_back(std::pair<string, Atom>(predicate, value));
		_metadata_orphans[subject_path] = l;
	}
}


void
Store::resolve_metadata_orphans(SharedPtr<ObjectModel> subject)
{
	map<Path, list<std::pair<string, Atom> > >::iterator v
		= _metadata_orphans.find(subject->path());

	if (v != _metadata_orphans.end()) {
	
		list<std::pair<string, Atom> > values = v->second; // take a copy

		_metadata_orphans.erase(subject->path());
		
		for (list<std::pair<string, Atom> >::iterator i = values.begin();
				i != values.end(); ++i) {
			subject->set_metadata(i->first, i->second);
		}
	}
}


void
Store::resolve_orphans(SharedPtr<ObjectModel> parent)
{
	map<Path, list<SharedPtr<ObjectModel> > >::iterator c
		= _orphans.find(parent->path());

	if (c != _orphans.end()) {
	
		list<SharedPtr<ObjectModel> > children = c->second; // take a copy

		_orphans.erase(parent->path()); // prevent infinite recursion
		
		for (list<SharedPtr<ObjectModel> >::iterator i = children.begin();
				i != children.end(); ++i) {
			add_object(*i);
		}
	}
}


void
Store::add_object(SharedPtr<ObjectModel> object)
{
	// If we already have "this" object, merge the existing one into the new
	// one (with precedence to the new values).
	ObjectMap::iterator existing = _objects.find(object->path());
	if (existing != _objects.end()) {
		existing->second->set(object);
	} else {

		if (object->path() != "/") {
			SharedPtr<ObjectModel> parent = this->object(object->path().parent());
			if (parent) {
				assert(object->path().is_child_of(parent->path()));
				object->set_parent(parent);
				parent->add_child(object);
				assert(parent && (object->parent() == parent));
				
				_objects[object->path()] = object;
				new_object_sig.emit(object);
				
				resolve_metadata_orphans(parent);
				resolve_orphans(parent);

				SharedPtr<PortModel> port = PtrCast<PortModel>(object);
				if (port)
					resolve_connection_orphans(port);

			} else {
				add_orphan(object);
			}
		} else {
			_objects[object->path()] = object;
			new_object_sig.emit(object);
		}

	}

	//cout << "[Store] Added " << object->path() << endl;
}


SharedPtr<ObjectModel>
Store::remove_object(const Path& path)
{
	map<Path, SharedPtr<ObjectModel> >::iterator i = _objects.find(path);

	if (i != _objects.end()) {
		assert((*i).second->path() == path);
		SharedPtr<ObjectModel> result = (*i).second;
		_objects.erase(i);
		//cout << "[Store] Removed " << path << endl;

		if (result)
			result->destroyed_sig.emit();

		if (result->path() != "/") {
			assert(result->parent());

			SharedPtr<ObjectModel> parent = this->object(result->path().parent());
			if (parent) {
				parent->remove_child(result);
			}
		}

		assert(!object(path));

		return result;

	} else {
		cerr << "[Store] Unable to find object " << path << " to remove." << endl;
		return SharedPtr<ObjectModel>();
	}
}


SharedPtr<PluginModel>
Store::plugin(const string& uri)
{
	assert(uri.length() > 0);
	map<string, SharedPtr<PluginModel> >::iterator i = _plugins.find(uri);
	if (i == _plugins.end())
		return SharedPtr<PluginModel>();
	else
		return (*i).second;
}


SharedPtr<ObjectModel>
Store::object(const Path& path)
{
	assert(path.length() > 0);
	map<Path, SharedPtr<ObjectModel> >::iterator i = _objects.find(path);
	if (i == _objects.end()) {
		return SharedPtr<ObjectModel>();
	} else {
		assert(i->second->path() == "/" || i->second->parent());
		return i->second;
	}
}

void
Store::add_plugin(SharedPtr<PluginModel> pm)
{
	// FIXME: dupes?  merge, like with objects?
	
	_plugins[pm->uri()] = pm;
}



/* ****** Signal Handlers ******** */


void
Store::destruction_event(const Path& path)
{
	SharedPtr<ObjectModel> removed = remove_object(path);

	removed.reset();

	//cerr << "Store removed object " << path
	//	<< ", count: " << removed.use_count();
}

void
Store::new_plugin_event(const string& uri, const string& type_uri, const string& name)
{
	SharedPtr<PluginModel> p(new PluginModel(uri, type_uri, name));
	add_plugin(p);
	resolve_plugin_orphans(p);
}


void
Store::new_patch_event(const Path& path, uint32_t poly)
{
	SharedPtr<PatchModel> p(new PatchModel(path, poly));
	add_object(p);
}


void
Store::new_node_event(const string& plugin_uri, const Path& node_path, bool is_polyphonic, uint32_t num_ports)
{
	// FIXME: num_ports unused
	
	SharedPtr<PluginModel> plug = plugin(plugin_uri);
	if (!plug) {
		SharedPtr<NodeModel> n(new NodeModel(plugin_uri, node_path, is_polyphonic));
		add_plugin_orphan(n);
	} else {
		SharedPtr<NodeModel> n(new NodeModel(plug, node_path, is_polyphonic));
		add_object(n);
	}
}


void
Store::new_port_event(const Path& path, const string& type, bool is_output)
{
	PortModel::Direction pdir = is_output ? PortModel::OUTPUT : PortModel::INPUT;

	SharedPtr<PortModel> p(new PortModel(path, type, pdir));
	add_object(p);
	if (p->parent())
		resolve_connection_orphans(p);
}


void
Store::patch_enabled_event(const Path& path)
{
	SharedPtr<PatchModel> patch = PtrCast<PatchModel>(object(path));
	if (patch)
		patch->enable();
}


void
Store::patch_disabled_event(const Path& path)
{
	SharedPtr<PatchModel> patch = PtrCast<PatchModel>(object(path));
	if (patch)
		patch->disable();
}


void
Store::patch_cleared_event(const Path& path)
{
	SharedPtr<PatchModel> patch = PtrCast<PatchModel>(object(path));
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
	SharedPtr<ObjectModel> subject = object(subject_path);
	
	if (subject) {
		subject->set_metadata(predicate, value);
	} else {
		add_metadata_orphan(subject_path, predicate, value);
		cerr << "WARNING: metadata for unknown object " << subject_path << endl;
	}
}


void
Store::control_change_event(const Path& port_path, float value)
{
	SharedPtr<PortModel> port = PtrCast<PortModel>(object(port_path));
	if (port)
		port->value(value);
	else
		cerr << "ERROR: control change for nonexistant port " << port_path << endl;
}


SharedPtr<PatchModel>
Store::connection_patch(const Path& src_port_path, const Path& dst_port_path)
{
	SharedPtr<PatchModel> patch;

	if (src_port_path.parent() == dst_port_path.parent())
		patch = PtrCast<PatchModel>(this->object(src_port_path.parent()));
	
	if (!patch && src_port_path.parent() == dst_port_path.parent().parent())
		patch = PtrCast<PatchModel>(this->object(src_port_path.parent()));

	if (!patch && src_port_path.parent().parent() == dst_port_path.parent())
		patch = PtrCast<PatchModel>(this->object(dst_port_path.parent()));
	
	if (!patch)
		patch = PtrCast<PatchModel>(this->object(src_port_path.parent().parent()));

	if (!patch)
		cerr << "ERROR: Unable to find connection patch " << src_port_path
			<< " -> " << dst_port_path << endl;

	return patch;
}


void
Store::connection_event(const Path& src_port_path, const Path& dst_port_path)
{
	SharedPtr<PortModel> src_port = PtrCast<PortModel>(object(src_port_path));
	SharedPtr<PortModel> dst_port = PtrCast<PortModel>(object(dst_port_path));

	SharedPtr<ConnectionModel> dangling_cm(new ConnectionModel(src_port_path, dst_port_path));

	if (src_port && dst_port) { 
	
		assert(src_port->parent());
		assert(dst_port->parent());

		SharedPtr<PatchModel> patch = connection_patch(src_port_path, dst_port_path);
		assert(patch);

		SharedPtr<ConnectionModel> cm(new ConnectionModel(src_port, dst_port));
		
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
	
	SharedPtr<PortModel> src_port = PtrCast<PortModel>(object(src_port_path));
	SharedPtr<PortModel> dst_port = PtrCast<PortModel>(object(dst_port_path));

	assert(src_port);
	assert(dst_port);
	
	src_port->disconnected_from(dst_port);
	dst_port->disconnected_from(src_port);

	SharedPtr<ConnectionModel> cm(new ConnectionModel(src_port, dst_port));

	SharedPtr<PatchModel> patch = connection_patch(src_port_path, dst_port_path);
	
	if (patch)
		patch->remove_connection(src_port_path, dst_port_path);
	else
		cerr << "ERROR: disconnection in nonexistant patch: "
			<< src_port_path << " -> " << dst_port_path << endl;
}


} // namespace Client
} // namespace Ingen

