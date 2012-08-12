/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ingen/client/ClientStore.hpp"
#include "ingen/client/EdgeModel.hpp"
#include "ingen/client/NodeModel.hpp"
#include "ingen/client/ObjectModel.hpp"
#include "ingen/client/PatchModel.hpp"
#include "ingen/client/PluginModel.hpp"
#include "ingen/client/PortModel.hpp"
#include "ingen/client/SigClientInterface.hpp"
#include "raul/PathTable.hpp"
#include "raul/log.hpp"

#define LOG(s) (s("[ClientStore] "))

// #define INGEN_CLIENT_STORE_DUMP 1

using namespace std;

namespace Ingen {
namespace Client {

ClientStore::ClientStore(URIs&                         uris,
                         SharedPtr<Interface>          engine,
                         SharedPtr<SigClientInterface> emitter)
	: _uris(uris)
	, _engine(engine)
	, _emitter(emitter)
	, _plugins(new Plugins())
{
	if (!emitter)
		return;

#define CONNECT(signal, method) \
	emitter->signal_##signal().connect( \
		sigc::mem_fun(this, &ClientStore::method));

	CONNECT(object_deleted, del);
	CONNECT(object_moved, move);
	CONNECT(put, put);
	CONNECT(delta, delta);
	CONNECT(connection, connect);
	CONNECT(disconnection, disconnect);
	CONNECT(disconnect_all, disconnect_all);
	CONNECT(property_change, set_property);
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
			SharedPtr<ObjectModel> parent = _object(object->path().parent());
			if (parent) {
				assert(object->path().is_child_of(parent->path()));
				object->set_parent(parent);
				parent->add_child(object);
				assert(parent && (object->parent() == parent));

				(*this)[object->path()] = object;
				_signal_new_object.emit(object);
			}
		} else {
			(*this)[object->path()] = object;
			_signal_new_object.emit(object);
		}

	}

	typedef Resource::Properties::const_iterator Iterator;
	for (Iterator i = object->properties().begin();
	     i != object->properties().end(); ++i)
		object->signal_property().emit(i->first, i->second);

	LOG(Raul::debug) << "Added " << object->path() << " {" << endl;
	for (iterator i = begin(); i != end(); ++i) {
		LOG(Raul::debug) << "\t" << i->first << endl;
	}
	LOG(Raul::debug) << "}" << endl;
}

SharedPtr<ObjectModel>
ClientStore::remove_object(const Raul::Path& path)
{
	iterator i = find(path);

	if (i != end()) {
		assert((*i).second->path() == path);
		SharedPtr<ObjectModel>    result  = PtrCast<ObjectModel>((*i).second);
		iterator                  end     = find_descendants_end(i);
		SharedPtr<Store::Objects> removed = yank(i, end);

		LOG(Raul::debug) << "Removing " << i->first << " {" << endl;
		for (iterator i = removed->begin(); i != removed->end(); ++i) {
			LOG(Raul::debug) << "\t" << i->first << endl;
		}
		LOG(Raul::debug) << "}" << endl;

		if (result)
			result->signal_destroyed().emit();

		if (!result->path().is_root()) {
			assert(result->parent());

			SharedPtr<ObjectModel> parent = _object(result->path().parent());
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
ClientStore::_plugin(const Raul::URI& uri)
{
	assert(uri.length() > 0);
	Plugins::iterator i = _plugins->find(uri);
	if (i == _plugins->end())
		return SharedPtr<PluginModel>();
	else
		return (*i).second;
}

SharedPtr<const PluginModel>
ClientStore::plugin(const Raul::URI& uri) const
{
	return const_cast<ClientStore*>(this)->_plugin(uri);
}

SharedPtr<ObjectModel>
ClientStore::_object(const Raul::Path& path)
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

SharedPtr<const ObjectModel>
ClientStore::object(const Raul::Path& path) const
{
	return const_cast<ClientStore*>(this)->_object(path);
}

SharedPtr<Resource>
ClientStore::_resource(const Raul::URI& uri)
{
	if (GraphObject::uri_is_path(uri)) {
		return _object(GraphObject::uri_to_path(uri));
	} else {
		return _plugin(uri);
	}
}

SharedPtr<const Resource>
ClientStore::resource(const Raul::URI& uri) const
{
	return const_cast<ClientStore*>(this)->_resource(uri);
}

void
ClientStore::add_plugin(SharedPtr<PluginModel> pm)
{
	SharedPtr<PluginModel> existing = _plugin(pm->uri());
	if (existing) {
		existing->set(pm);
	} else {
		_plugins->insert(make_pair(pm->uri(), pm));
		_signal_new_plugin.emit(pm);
	}
}

/* ****** Signal Handlers ******** */

void
ClientStore::del(const Raul::URI& uri)
{
	if (!GraphObject::uri_is_path(uri)) {
		return;
	}

	const Raul::Path path(uri.str());
	SharedPtr<ObjectModel> removed = remove_object(path);
	removed.reset();
	LOG(Raul::debug) << "Removed object " << path
	                 << ", count: " << removed.use_count();
}

void
ClientStore::move(const Raul::Path& old_path, const Raul::Path& new_path)
{
	iterator parent = find(old_path);
	if (parent == end()) {
		LOG(Raul::error) << "Failed to find object " << old_path
		                 << " to move." << endl;
		return;
	}

	typedef Raul::Table<Raul::Path, SharedPtr<GraphObject> > Removed;

	iterator           end     = find_descendants_end(parent);
	SharedPtr<Removed> removed = yank(parent, end);

	assert(removed->size() > 0);

	typedef Raul::Table<Raul::Path, SharedPtr<GraphObject> > PathTable;
	for (PathTable::iterator i = removed->begin(); i != removed->end(); ++i) {
		const Raul::Path& child_old_path = i->first;
		assert(Raul::Path::descendant_comparator(old_path, child_old_path));

		Raul::Path child_new_path;
		if (child_old_path == old_path)
			child_new_path = new_path;
		else
			child_new_path = new_path.base()
				+ child_old_path.substr(old_path.length() + 1);

		LOG(Raul::info)(Raul::fmt("Renamed %1% to %2%\n")
		                % child_old_path.c_str() % child_new_path.c_str());
		PtrCast<ObjectModel>(i->second)->set_path(child_new_path);
		i->first = child_new_path;
	}

	cram(*removed.get());
}

void
ClientStore::put(const Raul::URI&            uri,
                 const Resource::Properties& properties,
                 Resource::Graph             ctx)
{
	typedef Resource::Properties::const_iterator Iterator;
#ifdef INGEN_CLIENT_STORE_DUMP
	LOG(Raul::info) << "PUT " << uri << " {" << endl;
	for (Iterator i = properties.begin(); i != properties.end(); ++i)
		LOG(Raul::info) << '\t' << i->first << " = " << _uris.forge.str(i->second)
		          << " :: " << i->second.type() << endl;
	LOG(Raul::info) << "}" << endl;
#endif

	bool is_patch, is_node, is_port, is_output;
	Resource::type(uris(), properties,
	               is_patch, is_node, is_port, is_output);

	// Check if uri is a plugin
	Iterator t = properties.find(_uris.rdf_type);
	if (t != properties.end() && t->second.type() == _uris.forge.URI) {
		const Raul::Atom&  type        = t->second;
		const Raul::URI&   type_uri    = type.get_uri();
		const Plugin::Type plugin_type = Plugin::type_from_uri(type_uri);
		if (plugin_type == Plugin::Patch) {
			is_patch = true;
		} else if (plugin_type != Plugin::NIL) {
			SharedPtr<PluginModel> p(
				new PluginModel(uris(), uri, type_uri, properties));
			add_plugin(p);
			return;
		}
	}

	if (!GraphObject::uri_is_path(uri)) {
		LOG(Raul::error)(Raul::fmt("Put for unknown subject <%1%>\n")
		                 % uri.c_str());
		return;
	}

	const Raul::Path path(GraphObject::uri_to_path(uri));

	SharedPtr<ObjectModel> obj = PtrCast<ObjectModel>(_object(path));
	if (obj) {
		obj->set_properties(properties);
		return;
	}

	if (path.is_root()) {
		is_patch = true;
	}

	if (is_patch) {
		SharedPtr<PatchModel> model(new PatchModel(uris(), path));
		model->set_properties(properties);
		add_object(model);
	} else if (is_node) {
		const Iterator p = properties.find(_uris.ingen_prototype);
		SharedPtr<PluginModel> plug;
		if (p->second.is_valid() && p->second.type() == _uris.forge.URI) {
			if (!(plug = _plugin(p->second.get_uri()))) {
				LOG(Raul::warn)(Raul::fmt("Unable to find plugin <%1%>\n")
				                % p->second.get_uri());
				plug = SharedPtr<PluginModel>(
					new PluginModel(uris(),
					                p->second.get_uri(),
					                _uris.ingen_nil,
					                Resource::Properties()));
				add_plugin(plug);
			}

			SharedPtr<NodeModel> n(new NodeModel(uris(), plug, path));
			n->set_properties(properties);
			add_object(n);
		} else {
			LOG(Raul::warn)(Raul::fmt("Node %1% has no plugin\n")
			                % path.c_str());
		}
	} else if (is_port) {
		PortModel::Direction pdir = (is_output)
			? PortModel::OUTPUT
			: PortModel::INPUT;
		const Iterator i = properties.find(_uris.lv2_index);
		if (i != properties.end() && i->second.type() == _uris.forge.Int) {
			const uint32_t index = i->second.get_int32();
			SharedPtr<PortModel> p(
				new PortModel(uris(), path, index, pdir));
			p->set_properties(properties);
			add_object(p);
		} else {
			LOG(Raul::error) << "Port " << path << " has no index" << endl;
		}
	} else {
		LOG(Raul::warn)(Raul::fmt("Ignoring object %1% with unknown type\n")
		                % path.c_str());
	}
}

void
ClientStore::delta(const Raul::URI&            uri,
                   const Resource::Properties& remove,
                   const Resource::Properties& add)
{
	typedef Resource::Properties::const_iterator iterator;
#ifdef INGEN_CLIENT_STORE_DUMP
	LOG(Raul::info) << "DELTA " << uri << " {" << endl;
	for (iterator i = remove.begin(); i != remove.end(); ++i)
		LOG(Raul::info) << "    - " << i->first
		                << " = " << _uris.forge.str(i->second)
		                << " :: " << i->second.type() << endl;
	for (iterator i = add.begin(); i != add.end(); ++i)
		LOG(Raul::info) << "    + " << i->first
		                << " = " << _uris.forge.str(i->second)
		                << " :: " << i->second.type() << endl;
	LOG(Raul::info) << "}" << endl;
#endif

	if (!GraphObject::uri_is_path(uri)) {
		LOG(Raul::error)(Raul::fmt("Delta for unknown subject <%1%>\n")
		                 % uri.c_str());
		return;
	}

	const Raul::Path path(GraphObject::uri_to_path(uri));

	SharedPtr<ObjectModel> obj = _object(path);
	if (obj) {
		obj->remove_properties(remove);
		obj->add_properties(add);
	} else {
		LOG(Raul::warn)(Raul::fmt("Failed to find object `%1%'\n")
		                % path.c_str());
	}
}

void
ClientStore::set_property(const Raul::URI&  subject_uri,
                          const Raul::URI&  predicate,
                          const Raul::Atom& value)
{
	if (subject_uri == _uris.ingen_engine) {
		LOG(Raul::info)(Raul::fmt("Engine property <%1%> = %2%\n")
		                % predicate.c_str() % _uris.forge.str(value));
		return;
	}
	SharedPtr<Resource> subject = _resource(subject_uri);
	if (subject) {
		subject->set_property(predicate, value);
	} else {
		SharedPtr<PluginModel> plugin = _plugin(subject_uri);
		if (plugin)
			plugin->set_property(predicate, value);
		else
			LOG(Raul::warn)(Raul::fmt("Property <%1%> for unknown object %2%\n")
			                % predicate.c_str() % subject_uri.c_str());
	}
}

SharedPtr<PatchModel>
ClientStore::connection_patch(const Raul::Path& tail_path,
                              const Raul::Path& head_path)
{
	SharedPtr<PatchModel> patch;

	if (tail_path.parent() == head_path.parent())
		patch = PtrCast<PatchModel>(_object(tail_path.parent()));

	if (!patch && tail_path.parent() == head_path.parent().parent())
		patch = PtrCast<PatchModel>(_object(tail_path.parent()));

	if (!patch && tail_path.parent().parent() == head_path.parent())
		patch = PtrCast<PatchModel>(_object(head_path.parent()));

	if (!patch)
		patch = PtrCast<PatchModel>(_object(tail_path.parent().parent()));

	if (!patch)
		LOG(Raul::error) << "Unable to find connection patch " << tail_path
			<< " -> " << head_path << endl;

	return patch;
}

bool
ClientStore::attempt_connection(const Raul::Path& tail_path,
                                const Raul::Path& head_path)
{
	SharedPtr<PortModel> tail = PtrCast<PortModel>(_object(tail_path));
	SharedPtr<PortModel> head = PtrCast<PortModel>(_object(head_path));

	if (tail && head) {
		SharedPtr<PatchModel> patch = connection_patch(tail_path, head_path);
		SharedPtr<EdgeModel>  cm(new EdgeModel(tail, head));

		tail->connected_to(head);
		head->connected_to(tail);

		patch->add_edge(cm);
		return true;
	} else {
		LOG(Raul::warn) << "Failed to connect " << tail_path
		                << " => " << head_path << std::endl;
		return false;
	}
}

void
ClientStore::connect(const Raul::Path& src_path,
                     const Raul::Path& dst_path)
{
	attempt_connection(src_path, dst_path);
}

void
ClientStore::disconnect(const Raul::Path& src_path,
                        const Raul::Path& dst_path)
{
	SharedPtr<PortModel> tail = PtrCast<PortModel>(_object(src_path));
	SharedPtr<PortModel> head = PtrCast<PortModel>(_object(dst_path));

	if (tail)
		tail->disconnected_from(head);

	if (head)
		head->disconnected_from(tail);

	SharedPtr<PatchModel> patch = connection_patch(src_path, dst_path);
	if (patch)
		patch->remove_edge(tail.get(), head.get());
}

void
ClientStore::disconnect_all(const Raul::Path& parent_patch,
                            const Raul::Path& path)
{
	SharedPtr<PatchModel>  patch  = PtrCast<PatchModel>(_object(parent_patch));
	SharedPtr<ObjectModel> object = _object(path);

	if (!patch || !object) {
		LOG(Raul::error) << "Bad disconnect all notification " << path
		                 << " in " << parent_patch << std::endl;
		return;
	}

	const PatchModel::Edges edges = patch->edges();
	for (PatchModel::Edges::const_iterator i = edges.begin();
	     i != edges.end(); ++i) {
		SharedPtr<EdgeModel> c = PtrCast<EdgeModel>(i->second);
		if (c->tail()->parent() == object
		    || c->head()->parent() == object
		    || c->tail()->path() == path
		    || c->head()->path() == path) {
			c->tail()->disconnected_from(c->head());
			c->head()->disconnected_from(c->tail());
			patch->remove_edge(c->tail().get(), c->head().get());
		}
	}
}

} // namespace Client
} // namespace Ingen

