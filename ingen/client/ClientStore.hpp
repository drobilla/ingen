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

#ifndef INGEN_CLIENT_CLIENTSTORE_HPP
#define INGEN_CLIENT_CLIENTSTORE_HPP

#include <cassert>
#include <list>
#include <string>

#include "ingen/Interface.hpp"
#include "ingen/client/signal.hpp"
#include "ingen/shared/Store.hpp"
#include "raul/Path.hpp"
#include "raul/PathTable.hpp"
#include "raul/SharedPtr.hpp"
#include "raul/TableImpl.hpp"

namespace Raul { class Atom; }

namespace Ingen {

namespace Shared { class URIs; }

class GraphObject;

namespace Client {

class NodeModel;
class ObjectModel;
class PatchModel;
class PluginModel;
class PortModel;
class SigClientInterface;

/** Automatically manages models of objects in the engine.
 *
 * @ingroup IngenClient
 */
class ClientStore : public Shared::Store
                  , public Interface
                  , public INGEN_TRACKABLE {
public:
	ClientStore(
		Shared::URIs&                 uris,
		SharedPtr<Interface>          engine  = SharedPtr<Interface>(),
		SharedPtr<SigClientInterface> emitter = SharedPtr<SigClientInterface>());

	Raul::URI uri() const { return "ingen:ClientStore"; }

	SharedPtr<const ObjectModel> object(const Raul::Path& path) const;
	SharedPtr<const PluginModel> plugin(const Raul::URI& uri)   const;
	SharedPtr<const Resource>    resource(const Raul::URI& uri) const;

	void clear();

	typedef Raul::Table<Raul::URI, SharedPtr<PluginModel> > Plugins;
	SharedPtr<const Plugins> plugins() const                   { return _plugins; }
	SharedPtr<Plugins>       plugins()                         { return _plugins; }
	void                     set_plugins(SharedPtr<Plugins> p) { _plugins = p; }

	Shared::URIs& uris() { return _uris; }

	void put(const Raul::URI&            uri,
	         const Resource::Properties& properties,
	         Resource::Graph             ctx=Resource::DEFAULT);

	void delta(const Raul::URI&            uri,
	           const Resource::Properties& remove,
	           const Resource::Properties& add);

	void move(const Raul::Path& old_path,
	          const Raul::Path& new_path);

	void set_property(const Raul::URI&  subject_path,
	                  const Raul::URI&  predicate,
	                  const Raul::Atom& value);

	void connect(const Raul::Path& tail,
	             const Raul::Path& head);

	void disconnect(const Raul::Path& tail,
	                const Raul::Path& head);

	void disconnect_all(const Raul::Path& parent_patch,
	                    const Raul::Path& path);

	void del(const Raul::URI& uri);

	void set_response_id(int32_t id) {}
	void get(const Raul::URI& uri) {}
	void response(int32_t id, Status status, const std::string& subject) {}
	void error(const std::string& msg) {}

	INGEN_SIGNAL(new_object, void, SharedPtr<ObjectModel>);
	INGEN_SIGNAL(new_plugin, void, SharedPtr<PluginModel>);

private:
	void add(GraphObject* o) { throw; }

	SharedPtr<ObjectModel> _object(const Raul::Path& path);
	SharedPtr<PluginModel> _plugin(const Raul::URI& uri);
	SharedPtr<Resource>    _resource(const Raul::URI& uri);

	void add_object(SharedPtr<ObjectModel> object);
	SharedPtr<ObjectModel> remove_object(const Raul::Path& path);

	void add_plugin(SharedPtr<PluginModel> plugin);

	SharedPtr<PatchModel> connection_patch(const Raul::Path& tail_path,
	                                       const Raul::Path& head_path);

	void bundle_begin() {}
	void bundle_end()   {}

	// Slots for SigClientInterface signals
	bool attempt_connection(const Raul::Path& tail_path,
	                        const Raul::Path& head_path);

	Shared::URIs&                 _uris;
	SharedPtr<Interface>          _engine;
	SharedPtr<SigClientInterface> _emitter;

	SharedPtr<Plugins> _plugins; ///< Map, keyed by plugin URI
};

} // namespace Client
} // namespace Ingen

#endif // INGEN_CLIENT_CLIENTSTORE_HPP
