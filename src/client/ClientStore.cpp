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

#include "ingen/Log.hpp"
#include "ingen/client/ArcModel.hpp"
#include "ingen/client/BlockModel.hpp"
#include "ingen/client/ClientStore.hpp"
#include "ingen/client/GraphModel.hpp"
#include "ingen/client/ObjectModel.hpp"
#include "ingen/client/PluginModel.hpp"
#include "ingen/client/PortModel.hpp"
#include "ingen/client/SigClientInterface.hpp"

// #define INGEN_CLIENT_STORE_DUMP 1

using namespace std;

namespace Ingen {
namespace Client {

ClientStore::ClientStore(URIs&                    uris,
                         Log&                     log,
                         SPtr<Interface>          engine,
                         SPtr<SigClientInterface> emitter)
	: _uris(uris)
	, _log(log)
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
ClientStore::add_object(SPtr<ObjectModel> object)
{
	// If we already have "this" object, merge the existing one into the new
	// one (with precedence to the new values).
	iterator existing = find(object->path());
	if (existing != end()) {
		dynamic_ptr_cast<ObjectModel>(existing->second)->set(object);
	} else {
		if (!object->path().is_root()) {
			SPtr<ObjectModel> parent = _object(object->path().parent());
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
	for (auto p : object->properties())
		object->signal_property().emit(p.first, p.second);
}

SPtr<ObjectModel>
ClientStore::remove_object(const Raul::Path& path)
{
	// Find the object, the "top" of the tree to remove
	const iterator top = find(path);
	if (top == end()) {
		return SPtr<ObjectModel>();
	}

	// Remove the object and all its descendants
	Objects removed;
	remove(top, removed);

	// Notify everything that needs to know this object is going away
	SPtr<ObjectModel> object = dynamic_ptr_cast<ObjectModel>(top->second);
	if (object) {
		// Notify the program this object is going away
		object->signal_destroyed().emit();

		// Remove object from parent model if applicable
		if (object->parent()) {
			object->parent()->remove_child(object);
		}
	}

	return object;
}

SPtr<PluginModel>
ClientStore::_plugin(const Raul::URI& uri)
{
	Plugins::iterator i = _plugins->find(uri);
	if (i == _plugins->end())
		return SPtr<PluginModel>();
	else
		return (*i).second;
}

SPtr<const PluginModel>
ClientStore::plugin(const Raul::URI& uri) const
{
	return const_cast<ClientStore*>(this)->_plugin(uri);
}

SPtr<ObjectModel>
ClientStore::_object(const Raul::Path& path)
{
	const iterator i = find(path);
	if (i == end()) {
		return SPtr<ObjectModel>();
	} else {
		SPtr<ObjectModel> model = dynamic_ptr_cast<ObjectModel>(i->second);
		assert(model);
		assert(model->path().is_root() || model->parent());
		return model;
	}
}

SPtr<const ObjectModel>
ClientStore::object(const Raul::Path& path) const
{
	return const_cast<ClientStore*>(this)->_object(path);
}

SPtr<Resource>
ClientStore::_resource(const Raul::URI& uri)
{
	if (Node::uri_is_path(uri)) {
		return _object(Node::uri_to_path(uri));
	} else {
		return _plugin(uri);
	}
}

SPtr<const Resource>
ClientStore::resource(const Raul::URI& uri) const
{
	return const_cast<ClientStore*>(this)->_resource(uri);
}

void
ClientStore::add_plugin(SPtr<PluginModel> pm)
{
	SPtr<PluginModel> existing = _plugin(pm->uri());
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
	if (Node::uri_is_path(uri)) {
		remove_object(Node::uri_to_path(uri));
	}
}

void
ClientStore::move(const Raul::Path& old_path, const Raul::Path& new_path)
{
	const iterator top = find(old_path);
	if (top != end()) {
		rename(top, new_path);
	}
}

void
ClientStore::put(const Raul::URI&            uri,
                 const Resource::Properties& properties,
                 Resource::Graph             ctx)
{
	typedef Resource::Properties::const_iterator Iterator;
#ifdef INGEN_CLIENT_STORE_DUMP
	std::cerr << "Put " << uri << " {" << endl;
	for (auto p : properties)
		std::cerr << '\t' << p.first << " = " << _uris.forge.str(p.second)
		          << " :: " << p.second.type() << endl;
	std::cerr << "}" << endl;
#endif

	bool is_graph, is_block, is_port, is_output;
	Resource::type(uris(), properties,
	               is_graph, is_block, is_port, is_output);

	// Check if uri is a plugin
	Iterator t = properties.find(_uris.rdf_type);
	if (t != properties.end() && t->second.type() == _uris.forge.URI) {
		const Atom&        type(t->second);
		const Raul::URI    type_uri(type.ptr<char>());
		const Plugin::Type plugin_type(Plugin::type_from_uri(type_uri));
		if (plugin_type == Plugin::Graph) {
			is_graph = true;
		} else if (plugin_type != Plugin::NIL) {
			SPtr<PluginModel> p(
				new PluginModel(uris(), uri, type_uri, properties));
			add_plugin(p);
			return;
		}
	}

	if (!Node::uri_is_path(uri)) {
		_log.error(Raul::fmt("Put for unknown subject <%1%>\n")
		           % uri.c_str());
		return;
	}

	const Raul::Path path(Node::uri_to_path(uri));

	SPtr<ObjectModel> obj = dynamic_ptr_cast<ObjectModel>(_object(path));
	if (obj) {
		obj->set_properties(properties);
		return;
	}

	if (path.is_root()) {
		is_graph = true;
	}

	if (is_graph) {
		SPtr<GraphModel> model(new GraphModel(uris(), path));
		model->set_properties(properties);
		add_object(model);
	} else if (is_block) {
		const Iterator p = properties.find(_uris.ingen_prototype);
		SPtr<PluginModel> plug;
		if (p->second.is_valid() && p->second.type() == _uris.forge.URI) {
			if (!(plug = _plugin(Raul::URI(p->second.ptr<char>())))) {
				plug = SPtr<PluginModel>(
					new PluginModel(uris(),
					                Raul::URI(p->second.ptr<char>()),
					                _uris.ingen_nil,
					                Resource::Properties()));
				add_plugin(plug);
			}

			SPtr<BlockModel> bm(new BlockModel(uris(), plug, path));
			bm->set_properties(properties);
			add_object(bm);
		} else {
			_log.warn(Raul::fmt("Block %1% has no plugin\n")
			          % path.c_str());
		}
	} else if (is_port) {
		PortModel::Direction pdir = (is_output)
			? PortModel::Direction::OUTPUT
			: PortModel::Direction::INPUT;
		const Iterator i = properties.find(_uris.lv2_index);
		if (i != properties.end() && i->second.type() == _uris.forge.Int) {
			const uint32_t index = i->second.get<int32_t>();
			SPtr<PortModel> p(
				new PortModel(uris(), path, index, pdir));
			p->set_properties(properties);
			add_object(p);
		} else {
			_log.error(Raul::fmt("Port %1% has no index\n") % path);
		}
	} else {
		_log.warn(Raul::fmt("Ignoring object %1% with unknown type\n")
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
	std::cerr << "Delta " << uri << " {" << endl;
	for (auto r : remove)
		std::cerr << "    - " << r.first
		          << " = " << _uris.forge.str(r.second)
		          << " :: " << r.second.type() << endl;
	for (auto a : add)
		std::cerr << "    + " << a.first
		          << " = " << _uris.forge.str(a.second)
		          << " :: " << a.second.type() << endl;
	std::cerr << "}" << endl;
#endif

	if (uri == Raul::URI("ingen:/clients/this")) {
		// Client property, which we don't store (yet?)
		return;
	}

	if (!Node::uri_is_path(uri)) {
		_log.error(Raul::fmt("Delta for unknown subject <%1%>\n")
		           % uri.c_str());
		return;
	}

	const Raul::Path path(Node::uri_to_path(uri));

	SPtr<ObjectModel> obj = _object(path);
	if (obj) {
		obj->remove_properties(remove);
		obj->add_properties(add);
	} else {
		_log.warn(Raul::fmt("Failed to find object `%1%'\n")
		          % path.c_str());
	}
}

void
ClientStore::set_property(const Raul::URI& subject_uri,
                          const Raul::URI& predicate,
                          const Atom&      value)
{
	if (subject_uri == _uris.ingen_engine) {
		_log.info(Raul::fmt("Engine property <%1%> = %2%\n")
		          % predicate.c_str() % _uris.forge.str(value));
		return;
	}
	SPtr<Resource> subject = _resource(subject_uri);
	if (subject) {
		if (predicate == _uris.ingen_activity) {
			/* Activity is transient, trigger any live actions (like GUI
			   blinkenlights) but do not store the property. */
			subject->on_property(predicate, value);
		} else {
			subject->set_property(predicate, value);
		}
	} else {
		SPtr<PluginModel> plugin = _plugin(subject_uri);
		if (plugin) {
			plugin->set_property(predicate, value);
		} else if (predicate != _uris.ingen_activity) {
			_log.warn(Raul::fmt("Property <%1%> for unknown object %2%\n")
			          % predicate.c_str() % subject_uri.c_str());
		}
	}
}

SPtr<GraphModel>
ClientStore::connection_graph(const Raul::Path& tail_path,
                              const Raul::Path& head_path)
{
	SPtr<GraphModel> graph;

	if (tail_path.parent() == head_path.parent())
		graph = dynamic_ptr_cast<GraphModel>(_object(tail_path.parent()));

	if (!graph && tail_path.parent() == head_path.parent().parent())
		graph = dynamic_ptr_cast<GraphModel>(_object(tail_path.parent()));

	if (!graph && tail_path.parent().parent() == head_path.parent())
		graph = dynamic_ptr_cast<GraphModel>(_object(head_path.parent()));

	if (!graph)
		graph = dynamic_ptr_cast<GraphModel>(_object(tail_path.parent().parent()));

	if (!graph)
		_log.error(Raul::fmt("Unable to find graph for arc %1% => %2%\n")
		           % tail_path % head_path);

	return graph;
}

bool
ClientStore::attempt_connection(const Raul::Path& tail_path,
                                const Raul::Path& head_path)
{
	SPtr<PortModel> tail = dynamic_ptr_cast<PortModel>(_object(tail_path));
	SPtr<PortModel> head = dynamic_ptr_cast<PortModel>(_object(head_path));

	if (tail && head) {
		SPtr<GraphModel> graph = connection_graph(tail_path, head_path);
		SPtr<ArcModel>   arc(new ArcModel(tail, head));

		tail->connected_to(head);
		head->connected_to(tail);

		graph->add_arc(arc);
		return true;
	} else {
		_log.warn(Raul::fmt("Failed to connect %1% => %2%\n")
		          % tail_path % head_path);
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
	SPtr<PortModel> tail = dynamic_ptr_cast<PortModel>(_object(src_path));
	SPtr<PortModel> head = dynamic_ptr_cast<PortModel>(_object(dst_path));

	if (tail)
		tail->disconnected_from(head);

	if (head)
		head->disconnected_from(tail);

	SPtr<GraphModel> graph = connection_graph(src_path, dst_path);
	if (graph)
		graph->remove_arc(tail.get(), head.get());
}

void
ClientStore::disconnect_all(const Raul::Path& parent_graph,
                            const Raul::Path& path)
{
	SPtr<GraphModel>  graph  = dynamic_ptr_cast<GraphModel>(_object(parent_graph));
	SPtr<ObjectModel> object = _object(path);

	if (!graph || !object) {
		_log.error(Raul::fmt("Bad disconnect all notification %1% in %2%\n")
		           % path % parent_graph);
		return;
	}

	const GraphModel::Arcs arcs = graph->arcs();
	for (auto a : arcs) {
		SPtr<ArcModel> arc = dynamic_ptr_cast<ArcModel>(a.second);
		if (arc->tail()->parent() == object
		    || arc->head()->parent() == object
		    || arc->tail()->path() == path
		    || arc->head()->path() == path) {
			arc->tail()->disconnected_from(arc->head());
			arc->head()->disconnected_from(arc->tail());
			graph->remove_arc(arc->tail().get(), arc->head().get());
		}
	}
}

} // namespace Client
} // namespace Ingen

