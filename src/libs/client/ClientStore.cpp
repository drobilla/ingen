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
#include "ClientStore.hpp"
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


ClientStore::ClientStore(SharedPtr<EngineInterface> engine, SharedPtr<SigClientInterface> emitter)
	: _engine(engine)
	, _emitter(emitter)
	, _plugins(new Plugins())
{
	_handle_orphans = (engine && emitter);

	if (!emitter)
		return;

	emitter->signal_object_destroyed.connect(sigc::mem_fun(this, &ClientStore::destruction));
	emitter->signal_object_renamed.connect(sigc::mem_fun(this, &ClientStore::rename));
	emitter->signal_new_plugin.connect(sigc::mem_fun(this, &ClientStore::new_plugin));
	emitter->signal_new_patch.connect(sigc::mem_fun(this, &ClientStore::new_patch));
	emitter->signal_new_node.connect(sigc::mem_fun(this, &ClientStore::new_node));
	emitter->signal_new_port.connect(sigc::mem_fun(this, &ClientStore::new_port));
	emitter->signal_patch_cleared.connect(sigc::mem_fun(this, &ClientStore::patch_cleared));
	emitter->signal_connection.connect(sigc::mem_fun(this, &ClientStore::connect));
	emitter->signal_disconnection.connect(sigc::mem_fun(this, &ClientStore::disconnect));
	emitter->signal_variable_change.connect(sigc::mem_fun(this, &ClientStore::set_variable));
	emitter->signal_property_change.connect(sigc::mem_fun(this, &ClientStore::set_property));
	emitter->signal_port_value.connect(sigc::mem_fun(this, &ClientStore::set_port_value));
	emitter->signal_voice_value.connect(sigc::mem_fun(this, &ClientStore::set_voice_value));
	emitter->signal_port_activity.connect(sigc::mem_fun(this, &ClientStore::port_activity));
}


void
ClientStore::clear()
{
	Store::clear();
	_plugins->clear();
}


void
ClientStore::add_plugin_orphan(SharedPtr<NodeModel> node)
{
	if (!_handle_orphans)
		return;
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
ClientStore::resolve_plugin_orphans(SharedPtr<PluginModel> plugin)
{
	if (!_handle_orphans)
		return;
	Raul::Table<string, list<SharedPtr<NodeModel> > >::iterator n
		= _plugin_orphans.find(plugin->uri());

	if (n != _plugin_orphans.end()) {
	
		list<SharedPtr<NodeModel> > spawn = n->second; // take a copy
		cerr << "Missing dependant " << plugin->uri() << " received" << endl;

		_plugin_orphans.erase(plugin->uri()); // prevent infinite recursion
		
		for (list<SharedPtr<NodeModel> >::iterator i = spawn.begin();
				i != spawn.end(); ++i) {
			(*i)->_plugin = plugin;
			add_object(*i);
		}
	}
}


void
ClientStore::add_connection_orphan(std::pair<Path, Path> orphan)
{
	// Do this anyway, it's needed to get the connections for copy&paste
	//if (!_handle_orphans)
		//return;
	
	if (_handle_orphans)
		cerr << "WARNING: Orphan connection " << orphan.first
			<< " -> " << orphan.second << " received." << endl;
	
	_connection_orphans.push_back(orphan);
}


void
ClientStore::resolve_connection_orphans(SharedPtr<PortModel> port)
{
	if (!_handle_orphans)
		return;
	assert(port->parent());

	for (list< pair<Path, Path> >::iterator c = _connection_orphans.begin();
			c != _connection_orphans.end(); ) {
		
		list< pair<Path, Path> >::iterator next = c;
		++next;
		
		if (c->first == port->path() || c->second == port->path()) {
			cerr << "Missing dependant (" << c->first << " -> " << c->second << ") received" << endl;
			bool success = attempt_connection(c->first, c->second);
			if (success)
				_connection_orphans.erase(c);
		}

		c = next;
	}
}


void
ClientStore::add_orphan(SharedPtr<ObjectModel> child)
{
	if (!_handle_orphans)
		return;
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
ClientStore::add_variable_orphan(const Path& subject_path, const string& predicate, const Atom& value)
{
	if (!_handle_orphans)
		return;
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
ClientStore::resolve_variable_orphans(SharedPtr<ObjectModel> subject)
{
	if (!_handle_orphans)
		return;
	Raul::PathTable<list<std::pair<string, Atom> > >::iterator v
		= _variable_orphans.find(subject->path());

	if (v != _variable_orphans.end()) {
	
		list<std::pair<string, Atom> > values = v->second; // take a copy

		_variable_orphans.erase(subject->path());
		cerr << "Missing dependant " << subject->path() << " received" << endl;
		
		for (list<std::pair<string, Atom> >::iterator i = values.begin();
				i != values.end(); ++i) {
			subject->set_variable(i->first, i->second);
		}
	}
}


void
ClientStore::resolve_orphans(SharedPtr<ObjectModel> parent)
{
	if (!_handle_orphans)
		return;
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
ClientStore::add_object(SharedPtr<ObjectModel> object)
{
	// If we already have "this" object, merge the existing one into the new
	// one (with precedence to the new values).
	iterator existing = find(object->path());
	if (existing != end()) {
		cout << "WARNING: Object " << object->path() << " already exists in store" << endl;
		PtrCast<ObjectModel>(existing->second)->set(object);
	} else {

		if (object->path() != "/") {
			SharedPtr<ObjectModel> parent = this->object(object->path().parent());
			if (parent) {
				assert(object->path().is_child_of(parent->path()));
				object->set_parent(parent);
				parent->add_child(object);
				assert(parent && (object->parent() == parent));
				
				(*this)[object->path()] = object;
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
			(*this)[object->path()] = object;
			signal_new_object.emit(object);
		}

	}

	/*cout << "[Store] Added " << object->path() << " {" << endl;
	for (iterator i = begin(); i != end(); ++i) {
		cout << "\t" << i->first << endl;
	}
	cout << "}" << endl;*/
}


SharedPtr<ObjectModel>
ClientStore::remove_object(const Path& path)
{
	iterator i = find(path);

	if (i != end()) {
		assert((*i).second->path() == path);
		SharedPtr<ObjectModel> result = PtrCast<ObjectModel>((*i).second);
		assert(result);
		//erase(i);
		iterator descendants_end = find_descendants_end(i);
		SharedPtr<Store::Objects> removed = yank(i, descendants_end);

		/*cout << "[Store] Removing " << i->first << " {" << endl;
		for (iterator i = removed.begin(); i != removed.end(); ++i) {
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
ClientStore::plugin(const string& uri)
{
	assert(uri.length() > 0);
	Plugins::iterator i = _plugins->find(uri);
	if (i == _plugins->end())
		return SharedPtr<PluginModel>();
	else
		return (*i).second;
}


SharedPtr<ObjectModel>
ClientStore::object(const Path& path)
{
	assert(path.length() > 0);
	iterator i = find(path);
	if (i == end()) {
		return SharedPtr<ObjectModel>();
	} else {
		SharedPtr<ObjectModel> model = PtrCast<ObjectModel>(i->second);
		assert(model);
		assert(model->path() == "/" || model->parent());
		return model;
	}
}

void
ClientStore::add_plugin(SharedPtr<PluginModel> pm)
{
	// FIXME: dupes?  merge, like with objects?
	
	(*_plugins)[pm->uri()] = pm;
	signal_new_plugin(pm);
	//cerr << "Plugin: " << pm->uri() << ", # plugins: " << _plugins->size() << endl;
}


/* ****** Signal Handlers ******** */


void
ClientStore::destruction(const Path& path)
{
	SharedPtr<ObjectModel> removed = remove_object(path);

	removed.reset();

	/*cerr << "Store removed object " << path
		<< ", count: " << removed.use_count();*/
}

void
ClientStore::rename(const Path& old_path, const Path& new_path)
{
	iterator parent = find(old_path);
	if (parent == end()) {
		cerr << "[Store] Failed to find object " << old_path << " to rename." << endl;
		return;
	}

	iterator descendants_end = find_descendants_end(parent);
	
	SharedPtr< Table<Path, SharedPtr<Shared::GraphObject> > > removed
			= yank(parent, descendants_end);
	
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

	cram(*removed.get());

	//cerr << "[Store] Table:" << endl;
	//for (size_t i=0; i < removed.size(); ++i) {
	//	cerr << removed[i].first << "\t\t: " << removed[i].second << endl;
	//}
	/*for (iterator i = begin(); i != end(); ++i) {
		cerr << i->first << "\t\t: " << i->second << endl;
	}*/
}

void
ClientStore::new_plugin(const string& uri, const string& type_uri, const string& symbol, const string& name)
{
	SharedPtr<PluginModel> p(new PluginModel(uri, type_uri, symbol, name));
	add_plugin(p);
	resolve_plugin_orphans(p);
}


void
ClientStore::new_patch(const string& path, uint32_t poly)
{
	SharedPtr<PatchModel> p(new PatchModel(path, poly));
	add_object(p);
}


void
ClientStore::new_node(const string& path, const string& plugin_uri)
{
	SharedPtr<PluginModel> plug = plugin(plugin_uri);
	if (!plug) {
		SharedPtr<NodeModel> n(new NodeModel(plugin_uri, path));
		add_plugin_orphan(n);
	} else {
		SharedPtr<NodeModel> n(new NodeModel(plug, path));
		add_object(n);
	}
}


void
ClientStore::new_port(const string& path, uint32_t index, const string& type, bool is_output)
{
	PortModel::Direction pdir = is_output ? PortModel::OUTPUT : PortModel::INPUT;

	SharedPtr<PortModel> p(new PortModel(path, index, type, pdir));
	add_object(p);
	if (p->parent())
		resolve_connection_orphans(p);
}


void
ClientStore::patch_cleared(const Path& path)
{
	iterator i = find(path);
	if (i != end()) {
		assert((*i).second->path() == path);
		SharedPtr<PatchModel> patch = PtrCast<PatchModel>(i->second);
		
		iterator first_descendant = i;
		++first_descendant;
		iterator descendants_end = find_descendants_end(i);
		SharedPtr< Table<Path, SharedPtr<Shared::GraphObject> > > removed
				= yank(first_descendant, descendants_end);

		for (iterator i = removed->begin(); i != removed->end(); ++i) {
			SharedPtr<ObjectModel> model = PtrCast<ObjectModel>(i->second);
			assert(model);
			model->signal_destroyed.emit();
			if (model->parent() == patch)
				patch->remove_child(model);
		}

	} else {
		cerr << "[Store] Unable to find patch " << path << " to clear." << endl;
	}
}


void
ClientStore::set_variable(const string& subject_path, const string& predicate, const Atom& value)
{
	SharedPtr<ObjectModel> subject = object(subject_path);

	if (!value.is_valid()) {
		cerr << "ERROR: variable '" << predicate << "' has no type" << endl;
	} else if (subject) {
		subject->set_variable(predicate, value);
	} else {
		add_variable_orphan(subject_path, predicate, value);
		cerr << "WARNING: variable for unknown object " << subject_path << endl;
	}
}

	
void
ClientStore::set_property(const string& subject_path, const string& predicate, const Atom& value)
{
	SharedPtr<ObjectModel> subject = object(subject_path);

	if (!value.is_valid()) {
		cerr << "ERROR: property '" << predicate << "' has no type" << endl;
	} else if (subject) {
		subject->set_property(predicate, value);
	} else {
		cerr << "WARNING: property for unknown object " << subject_path
				<< " lost.  Client must refresh!" << endl;
	}
}


void
ClientStore::set_port_value(const string& port_path, const Raul::Atom& value)
{
	SharedPtr<PortModel> port = PtrCast<PortModel>(object(port_path));
	if (port)
		port->value(value);
	else
		cerr << "ERROR: control change for nonexistant port " << port_path << endl;
}


void
ClientStore::set_voice_value(const string& port_path, uint32_t voice, const Raul::Atom& value)
{
	SharedPtr<PortModel> port = PtrCast<PortModel>(object(port_path));
	if (port)
		port->value(voice, value);
	else
		cerr << "ERROR: poly control change for nonexistant port " << port_path << endl;
}

	
void
ClientStore::port_activity(const Path& port_path)
{
	SharedPtr<PortModel> port = PtrCast<PortModel>(object(port_path));
	if (port)
		port->signal_activity.emit();
	else
		cerr << "ERROR: activity for nonexistant port " << port_path << endl;
}


SharedPtr<PatchModel>
ClientStore::connection_patch(const Path& src_port_path, const Path& dst_port_path)
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
ClientStore::attempt_connection(const Path& src_port_path, const Path& dst_port_path, bool add_orphan)
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
ClientStore::connect(const string& src_port_path, const string& dst_port_path)
{
	attempt_connection(src_port_path, dst_port_path, true);
}


void
ClientStore::disconnect(const string& src_port_path, const string& dst_port_path)
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

