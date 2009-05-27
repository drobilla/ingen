/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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

#include "raul/PathTable.hpp"
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
using namespace Shared;
namespace Client {


ClientStore::ClientStore(SharedPtr<EngineInterface> engine, SharedPtr<SigClientInterface> emitter)
	: _engine(engine)
	, _emitter(emitter)
	, _plugins(new Plugins())
{
	_handle_orphans = (engine && emitter);

	if (!emitter)
		return;

	emitter->signal_object_destroyed.connect(sigc::mem_fun(this, &ClientStore::del));
	emitter->signal_object_moved.connect(sigc::mem_fun(this, &ClientStore::move));
	emitter->signal_new_plugin.connect(sigc::mem_fun(this, &ClientStore::new_plugin));
	emitter->signal_put.connect(sigc::mem_fun(this, &ClientStore::put));
	emitter->signal_clear_patch.connect(sigc::mem_fun(this, &ClientStore::clear_patch));
	emitter->signal_connection.connect(sigc::mem_fun(this, &ClientStore::connect));
	emitter->signal_disconnection.connect(sigc::mem_fun(this, &ClientStore::disconnect));
	emitter->signal_variable_change.connect(sigc::mem_fun(this, &ClientStore::set_variable));
	emitter->signal_property_change.connect(sigc::mem_fun(this, &ClientStore::set_property));
	emitter->signal_port_value.connect(sigc::mem_fun(this, &ClientStore::set_port_value));
	emitter->signal_voice_value.connect(sigc::mem_fun(this, &ClientStore::set_voice_value));
	emitter->signal_activity.connect(sigc::mem_fun(this, &ClientStore::activity));
}


void
ClientStore::clear()
{
	Store::clear();
	_plugins->clear();
}


void
ClientStore::add_object(SharedPtr<ObjectModel> object)
{
	// If we already have "this" object, merge the existing one into the new
	// one (with precedence to the new values).
	iterator existing = find(object->path());
	if (existing != end()) {
		PtrCast<ObjectModel>(existing->second)->set(object);
	} else {
		if (!object->path().is_root()) {
			SharedPtr<ObjectModel> parent = this->object(object->path().parent());
			if (parent) {
				assert(object->path().is_child_of(parent->path()));
				object->set_parent(parent);
				parent->add_child(object);
				assert(parent && (object->parent() == parent));

				(*this)[object->path()] = object;
				signal_new_object.emit(object);

#if 0
				resolve_variable_orphans(parent);
				resolve_orphans(parent);

				SharedPtr<PortModel> port = PtrCast<PortModel>(object);
				if (port)
					resolve_connection_orphans(port);
#endif

			} else {
				//add_orphan(object);
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

		if (!result->path().is_root()) {
			assert(result->parent());

			SharedPtr<ObjectModel> parent = this->object(result->path().parent());
			if (parent) {
				parent->remove_child(result);
			}
		}

		assert(!object(path));

		return result;

	} else {
		return SharedPtr<ObjectModel>();
	}
}


SharedPtr<PluginModel>
ClientStore::plugin(const URI& uri)
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
		assert(model->path().is_root() || model->parent());
		return model;
	}
}

SharedPtr<Resource>
ClientStore::resource(const URI& uri)
{
	if (uri.scheme() == Path::scheme && Path::is_valid(uri.str()))
		return object(uri.str());
	else
		return plugin(uri);
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
ClientStore::del(const Path& path)
{
	SharedPtr<ObjectModel> removed = remove_object(path);
	removed.reset();
	//cerr << "[ClientStore] removed object " << path << ", count: " << removed.use_count();
}

void
ClientStore::move(const Path& old_path_str, const Path& new_path_str)
{
	Path old_path(old_path_str);
	Path new_path(new_path_str);

	iterator parent = find(old_path);
	if (parent == end()) {
		cerr << "[Store] Failed to find object " << old_path << " to move." << endl;
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
ClientStore::new_plugin(const URI& uri, const URI& type_uri, const Symbol& symbol)
{
	SharedPtr<PluginModel> p(new PluginModel(uri, type_uri));
	p->set_property("lv2:symbol", Atom(Atom::STRING, symbol));
	add_plugin(p);
	//resolve_plugin_orphans(p);
}


void
ClientStore::put(const Path& path, const Resource::Properties& properties)
{
	typedef Resource::Properties::const_iterator iterator;
	cerr << "CLIENT PUT " << path << " {" << endl;
	for (iterator i = properties.begin(); i != properties.end(); ++i)
		cerr << "\t" << i->first << " = " << i->second << " :: " << i->second.type() << endl;
	cerr << "}" << endl;

	bool is_patch, is_node, is_port, is_output;
	DataType data_type(DataType::UNKNOWN);
	ResourceImpl::type(properties, is_patch, is_node, is_port, is_output, data_type);

	if (is_patch) {
		uint32_t poly = 1;
		iterator p = properties.find("ingen:polyphony");
		if (p != properties.end() && p->second.is_valid() && p->second.type() == Atom::INT)
			poly = p->second.get_int32();
		SharedPtr<PatchModel> model(new PatchModel(path, poly));
		model->merge_properties(properties);
		add_object(model);
	} else if (is_node) {
		const Resource::Properties::const_iterator p = properties.find("rdf:instanceOf");
		SharedPtr<PluginModel> plug;
		if (p->second.is_valid() && p->second.type() == Atom::URI) {
			if ((plug = plugin(p->second.get_uri()))) {
				SharedPtr<NodeModel> n(new NodeModel(plug, path));
				n->merge_properties(properties);
				add_object(n);
			} else {
				SharedPtr<NodeModel> n(new NodeModel(p->second.get_uri(), path));
				n->merge_properties(properties);
				//add_plugin_orphan(n);
				add_object(n);
			}
		} else {
			cerr << "ERROR: Plugin with no type" << endl;
		}
	} else if (is_port) {
		if (data_type != DataType::UNKNOWN) {
			PortModel::Direction pdir = is_output ? PortModel::OUTPUT : PortModel::INPUT;
			SharedPtr<PortModel> p(new PortModel(path, 0, data_type, pdir));
			p->merge_properties(properties);
			add_object(p);
		} else {
			cerr << "WARNING: Illegal port " << path << endl;
		}
	} else {
		cerr << "WARNING: Ignoring object " << path << " with unknown type "
			<< is_patch << " " << is_node << " " << is_port << endl;
	}
}


void
ClientStore::clear_patch(const Path& path)
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
ClientStore::set_variable(const URI& subject_path, const URI& predicate, const Atom& value)
{
	SharedPtr<Resource> subject = resource(subject_path);

	if (!value.is_valid()) {
		cerr << "ERROR: variable '" << predicate << "' is invalid" << endl;
	} else if (subject) {
		SharedPtr<ObjectModel> om = PtrCast<ObjectModel>(subject);
		if (om)
			om->set_variable(predicate, value);
		else
			subject->set_property(predicate, value);
	} else {
		//add_variable_orphan(subject_path, predicate, value);
		cerr << "WARNING: variable '" << predicate << "' for unknown object " << subject_path << endl;
	}
}


void
ClientStore::set_property(const URI& subject_path, const URI& predicate, const Atom& value)
{
	SharedPtr<Resource> subject = resource(subject_path);
	if (!value.is_valid()) {
		cerr << "ERROR: property '" << predicate << "' is invalid" << endl;
	} else if (subject) {
		subject->set_property(predicate, value);
	} else {
		cerr << "WARNING: property '" << predicate << "' for unknown object " << subject_path << endl;
	}
}


void
ClientStore::set_port_value(const Path& port_path, const Atom& value)
{
	SharedPtr<PortModel> port = PtrCast<PortModel>(object(port_path));
	if (port)
		port->value(value);
	else
		cerr << "ERROR: control change for nonexistant port " << port_path << endl;
}


void
ClientStore::set_voice_value(const Path& port_path, uint32_t voice, const Atom& value)
{
	SharedPtr<PortModel> port = PtrCast<PortModel>(object(port_path));
	if (port)
		port->value(voice, value);
	else
		cerr << "ERROR: poly control change for nonexistant port " << port_path << endl;
}


void
ClientStore::activity(const Path& path)
{
	SharedPtr<PortModel> port = PtrCast<PortModel>(object(path));
	if (port)
		port->signal_activity.emit();
	else
		cerr << "ERROR: activity for nonexistant port " << path << endl;
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
		//add_connection_orphan(make_pair(src_port_path, dst_port_path));
	}

	return false;
}


void
ClientStore::connect(const Path& src_port_path, const Path& dst_port_path)
{
	attempt_connection(src_port_path, dst_port_path, true);
}


void
ClientStore::disconnect(const Path& src_port_path, const Path& dst_port_path)
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

