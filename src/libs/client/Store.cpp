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

#include <raul/PathTable.hpp>
#include "Store.hpp"
#include "ObjectModel.hpp"
#include "PatchModel.hpp"
#include "NodeModel.hpp"
#include "PortModel.hpp"
#include "PluginModel.hpp"
#include "PatchModel.hpp"
#include "SigClientInterface.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Client {


Store::Store(SharedPtr<EngineInterface> engine, SharedPtr<SigClientInterface> emitter)
: _engine(engine)
, _emitter(emitter)
{
	emitter->signal_object_destroyed.connect(sigc::mem_fun(this, &Store::destruction_event));
	emitter->signal_object_renamed.connect(sigc::mem_fun(this, &Store::rename_event));
	emitter->signal_new_plugin.connect(sigc::mem_fun(this, &Store::new_plugin_event));
	emitter->signal_new_patch.connect(sigc::mem_fun(this, &Store::new_patch_event));
	emitter->signal_new_node.connect(sigc::mem_fun(this, &Store::new_node_event));
	emitter->signal_new_port.connect(sigc::mem_fun(this, &Store::new_port_event));
	emitter->signal_polyphonic.connect(sigc::mem_fun(this, &Store::polyphonic_event));
	emitter->signal_patch_enabled.connect(sigc::mem_fun(this, &Store::patch_enabled_event));
	emitter->signal_patch_disabled.connect(sigc::mem_fun(this, &Store::patch_disabled_event));
	emitter->signal_patch_polyphony.connect(sigc::mem_fun(this, &Store::patch_polyphony_event));
	emitter->signal_patch_cleared.connect(sigc::mem_fun(this, &Store::patch_cleared_event));
	emitter->signal_connection.connect(sigc::mem_fun(this, &Store::connection_event));
	emitter->signal_disconnection.connect(sigc::mem_fun(this, &Store::disconnection_event));
	emitter->signal_variable_change.connect(sigc::mem_fun(this, &Store::variable_change_event));
	emitter->signal_control_change.connect(sigc::mem_fun(this, &Store::control_change_event));
	emitter->signal_port_activity.connect(sigc::mem_fun(this, &Store::port_activity_event));
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

	Raul::Table<string, list<SharedPtr<NodeModel> > >::iterator spawn
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
	Raul::Table<string, list<SharedPtr<NodeModel> > >::iterator n
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
Store::add_connection_orphan(std::pair<Path, Path> orphan)
{
	cerr << "WARNING: Orphan connection " << orphan.first
		<< " -> " << orphan.second << " received." << endl;
	
	_connection_orphans.push_back(orphan);
}


void
Store::resolve_connection_orphans(SharedPtr<PortModel> port)
{
	assert(port->parent());

	for (list< pair<Path, Path> >::iterator c = _connection_orphans.begin();
			c != _connection_orphans.end(); ) {
		
		list< pair<Path, Path> >::iterator next = c;
		++next;
		
		if (c->first == port->path() || c->second == port->path()) {
			bool success = attempt_connection(c->first, c->second);
			if (success)
				_connection_orphans.erase(c);
		}

		c = next;
	}
}


void
Store::add_orphan(SharedPtr<ObjectModel> child)
{
	cerr << "WARNING: Orphan object " << child->path() << " received." << endl;

	Raul::PathTable<list<SharedPtr<ObjectModel> > >::iterator children
		= _orphans.find(child->path().parent());

	_engine->request_object(child->path().parent());

	if (children != _orphans.end()) {
		children->second.push_back(child);
	} else {
		list<SharedPtr<ObjectModel> > l;
		l.push_back(child);
		_orphans.insert(make_pair(child->path().parent(), l));
	}
}


void
Store::add_variable_orphan(const Path& subject_path, const string& predicate, const Atom& value)
{
	Raul::PathTable<list<std::pair<string, Atom> > >::iterator orphans
		= _variable_orphans.find(subject_path);

	_engine->request_object(subject_path);

	if (orphans != _variable_orphans.end()) {
		orphans->second.push_back(std::pair<string, Atom>(predicate, value));
	} else {
		list<std::pair<string, Atom> > l;
		l.push_back(std::pair<string, Atom>(predicate, value));
		_variable_orphans[subject_path] = l;
	}
}


void
Store::resolve_variable_orphans(SharedPtr<ObjectModel> subject)
{
	Raul::PathTable<list<std::pair<string, Atom> > >::iterator v
		= _variable_orphans.find(subject->path());

	if (v != _variable_orphans.end()) {
	
		list<std::pair<string, Atom> > values = v->second; // take a copy

		_variable_orphans.erase(subject->path());
		
		for (list<std::pair<string, Atom> >::iterator i = values.begin();
				i != values.end(); ++i) {
			subject->set_variable(i->first, i->second);
		}
	}
}


void
Store::resolve_orphans(SharedPtr<ObjectModel> parent)
{
	Raul::PathTable<list<SharedPtr<ObjectModel> > >::iterator c
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
	Objects::iterator existing = _objects.find(object->path());
	if (existing != _objects.end()) {
		PtrCast<ObjectModel>(existing->second)->set(object);
	} else {

		if (object->path() != "/") {
			SharedPtr<ObjectModel> parent = this->object(object->path().parent());
			if (parent) {
				assert(object->path().is_child_of(parent->path()));
				object->set_parent(parent);
				parent->add_child(object);
				assert(parent && (object->parent() == parent));
				
				_objects[object->path()] = object;
				signal_new_object.emit(object);
				
				resolve_variable_orphans(parent);
				resolve_orphans(parent);

				SharedPtr<PortModel> port = PtrCast<PortModel>(object);
				if (port)
					resolve_connection_orphans(port);

			} else {
				add_orphan(object);
			}
		} else {
			_objects[object->path()] = object;
			signal_new_object.emit(object);
		}

	}

	/*cout << "[Store] Added " << object->path() << " {" << endl;
	for (Objects::iterator i = _objects.begin(); i != _objects.end(); ++i) {
		cout << "\t" << i->first << endl;
	}
	cout << "}" << endl;*/
}


SharedPtr<ObjectModel>
Store::remove_object(const Path& path)
{
	Objects::iterator i = _objects.find(path);

	if (i != _objects.end()) {
		assert((*i).second->path() == path);
		SharedPtr<ObjectModel> result = PtrCast<ObjectModel>((*i).second);
		assert(result);
		//_objects.erase(i);
		Objects::iterator descendants_end = _objects.find_descendants_end(i);
		SharedPtr< Table<Path, SharedPtr<Shared::GraphObject> > > removed
				= _objects.yank(i, descendants_end);

		/*cout << "[Store] Removing " << i->first << " {" << endl;
		for (Objects::iterator i = removed.begin(); i != removed.end(); ++i) {
			cout << "\t" << i->first << endl;
		}
		cout << "}" << endl;*/

		if (result)
			result->signal_destroyed.emit();

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
	Plugins::iterator i = _plugins.find(uri);
	if (i == _plugins.end())
		return SharedPtr<PluginModel>();
	else
		return (*i).second;
}


SharedPtr<ObjectModel>
Store::object(const Path& path)
{
	assert(path.length() > 0);
	Objects::iterator i = _objects.find(path);
	if (i == _objects.end()) {
		return SharedPtr<ObjectModel>();
	} else {
		SharedPtr<ObjectModel> model = PtrCast<ObjectModel>(i->second);
		assert(model);
		assert(model->path() == "/" || model->parent());
		return model;
	}
}

void
Store::add_plugin(SharedPtr<PluginModel> pm)
{
	// FIXME: dupes?  merge, like with objects?
	
	_plugins[pm->uri()] = pm;
	//cerr << "Plugin: " << pm->uri() << ", # plugins: " << _plugins.size() << endl;
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
Store::rename_event(const Path& old_path, const Path& new_path)
{
	Objects::iterator parent = _objects.find(old_path);
	if (parent == _objects.end()) {
		cerr << "[Store] Failed to find object " << old_path << " to rename." << endl;
		return;
	}

	Objects::iterator descendants_end = _objects.find_descendants_end(parent);
	
	SharedPtr< Table<Path, SharedPtr<Shared::GraphObject> > > removed
			= _objects.yank(parent, descendants_end);
	
	assert(removed->size() > 0);

	for (Table<Path, SharedPtr<Shared::GraphObject> >::iterator i = removed->begin(); i != removed->end(); ++i) {
		const Path& child_old_path = i->first;
		assert(Path::descendant_comparator(old_path, child_old_path));
		
		Path child_new_path;
		if (child_old_path == old_path)
			child_new_path = new_path;
		else
			child_new_path = new_path.base() + child_old_path.substr(old_path.length()+1);

		cerr << "[Store] Renamed " << child_old_path << " -> " << child_new_path << endl;
		PtrCast<ObjectModel>(i->second)->set_path(child_new_path);
		i->first = child_new_path;
	}

	_objects.cram(*removed.get());

	//cerr << "[Store] Table:" << endl;
	//for (size_t i=0; i < removed.size(); ++i) {
	//	cerr << removed[i].first << "\t\t: " << removed[i].second << endl;
	//}
	/*for (Objects::iterator i = _objects.begin(); i != _objects.end(); ++i) {
		cerr << i->first << "\t\t: " << i->second << endl;
	}*/
}

void
Store::new_plugin_event(const string& uri, const string& type_uri, const string& symbol, const string& name)
{
	SharedPtr<PluginModel> p(new PluginModel(uri, type_uri, symbol, name));
	add_plugin(p);
	resolve_plugin_orphans(p);
}


void
Store::new_patch_event(const Path& path, uint32_t poly)
{
	SharedPtr<PatchModel> p(new PatchModel(*this, path, poly));
	add_object(p);
}


void
Store::new_node_event(const string& plugin_uri, const Path& node_path, bool is_polyphonic, uint32_t num_ports)
{
	// FIXME: num_ports unused
	
	SharedPtr<PluginModel> plug = plugin(plugin_uri);
	if (!plug) {
		SharedPtr<NodeModel> n(new NodeModel(*this, plugin_uri, node_path, is_polyphonic));
		add_plugin_orphan(n);
	} else {
		SharedPtr<NodeModel> n(new NodeModel(*this, plug, node_path, is_polyphonic));
		add_object(n);
	}
}


void
Store::new_port_event(const Path& path, uint32_t index, const string& type, bool is_output)
{
	PortModel::Direction pdir = is_output ? PortModel::OUTPUT : PortModel::INPUT;

	SharedPtr<PortModel> p(new PortModel(*this, path, index, type, pdir));
	add_object(p);
	if (p->parent())
		resolve_connection_orphans(p);
}

	
void
Store::polyphonic_event(const Path& path, bool polyphonic)
{
	SharedPtr<ObjectModel> object = this->object(path);
	if (object)
		object->set_polyphonic(polyphonic);
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
Store::patch_polyphony_event(const Path& path, uint32_t poly)
{
	SharedPtr<PatchModel> patch = PtrCast<PatchModel>(object(path));
	if (patch)
		patch->poly(poly);
}


void
Store::patch_cleared_event(const Path& path)
{
	SharedPtr<PatchModel> patch = PtrCast<PatchModel>(object(path));
	if (patch)
		for (ObjectModel::const_iterator i = patch->children_begin(); i != patch->children_end(); ++i)
			if (i->second->graph_parent() == patch.get())
				destruction_event(i->second->path());
}


void
Store::variable_change_event(const Path& subject_path, const string& predicate, const Atom& value)
{
	SharedPtr<ObjectModel> subject = object(subject_path);

	if (!value) {
		cerr << "ERROR: variable '" << predicate << "' has no type" << endl;
	} else if (subject) {
		cerr << "Set variable '" << predicate << "' with type " << (int)value.type() << endl;
		subject->set_variable(predicate, value);
	} else {
		add_variable_orphan(subject_path, predicate, value);
		cerr << "WARNING: variable for unknown object " << subject_path << endl;
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

	
void
Store::port_activity_event(const Path& port_path)
{
	SharedPtr<PortModel> port = PtrCast<PortModel>(object(port_path));
	if (port)
		port->signal_activity.emit();
	else
		cerr << "ERROR: activity for nonexistant port " << port_path << endl;
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


bool
Store::attempt_connection(const Path& src_port_path, const Path& dst_port_path, bool add_orphan)
{
	SharedPtr<PortModel> src_port = PtrCast<PortModel>(object(src_port_path));
	SharedPtr<PortModel> dst_port = PtrCast<PortModel>(object(dst_port_path));

	if (src_port && dst_port) { 
	
		assert(src_port->parent());
		assert(dst_port->parent());

		SharedPtr<PatchModel> patch = connection_patch(src_port_path, dst_port_path);
		assert(patch);

		SharedPtr<ConnectionModel> cm(new ConnectionModel(src_port, dst_port));
		
		src_port->connected_to(dst_port);
		dst_port->connected_to(src_port);

		patch->add_connection(cm);

		return true;
	
	} else if (add_orphan) {

		add_connection_orphan(make_pair(src_port_path, dst_port_path));

	}

	return false;
}

	
void
Store::connection_event(const Path& src_port_path, const Path& dst_port_path)
{
	attempt_connection(src_port_path, dst_port_path, true);
}


void
Store::disconnection_event(const Path& src_port_path, const Path& dst_port_path)
{
	// Find the ports and create a ConnectionModel just to get at the parent path
	// finding logic in ConnectionModel.  So I'm lazy.
	
	SharedPtr<PortModel> src_port = PtrCast<PortModel>(object(src_port_path));
	SharedPtr<PortModel> dst_port = PtrCast<PortModel>(object(dst_port_path));

	if (src_port)
		src_port->disconnected_from(dst_port);
	else
		cerr << "WARNING: Disconnection from nonexistant src port " << src_port_path << endl;
	
	if (dst_port)
		dst_port->disconnected_from(dst_port);
	else
		cerr << "WARNING: Disconnection from nonexistant dst port " << dst_port_path << endl;

	SharedPtr<PatchModel> patch = connection_patch(src_port_path, dst_port_path);
	
	if (patch)
		patch->remove_connection(src_port_path, dst_port_path);
	else
		cerr << "ERROR: disconnection in nonexistant patch: "
			<< src_port_path << " -> " << dst_port_path << endl;
}


} // namespace Client
} // namespace Ingen

