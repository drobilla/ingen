/*
  This file is part of Ingen.
  Copyright 2007-2016 David Robillard <http://drobilla.net/>

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

#include "ingen/Interface.hpp"
#include "ingen/Message.hpp"
#include "ingen/Store.hpp"
#include "ingen/URI.hpp"
#include "ingen/client/signal.hpp"
#include "ingen/ingen.h"

#include <map>
#include <memory>
#include <utility>

namespace raul {
class Path;
} // namespace raul

namespace ingen {

class Atom;
class Log;
class Resource;
class URIs;

namespace client {

class GraphModel;
class ObjectModel;
class PluginModel;
class SigClientInterface;

/** Automatically manages models of objects in the engine.
 *
 * @ingroup IngenClient
 */
class INGEN_API ClientStore : public Store
                            , public Interface
                            , public INGEN_TRACKABLE {
public:
	ClientStore(URIs&                                      uris,
	            Log&                                       log,
	            const std::shared_ptr<SigClientInterface>& emitter =
	                std::shared_ptr<SigClientInterface>());

	URI uri() const override { return URI("ingen:/clients/store"); }

	std::shared_ptr<const ObjectModel> object(const raul::Path& path) const;
	std::shared_ptr<const PluginModel> plugin(const URI& uri)   const;
	std::shared_ptr<const Resource>    resource(const URI& uri) const;

	void clear();

	using Plugins = std::map<const URI, std::shared_ptr<PluginModel>>;

	std::shared_ptr<const Plugins> plugins() const { return _plugins; }
	std::shared_ptr<Plugins>       plugins() { return _plugins; }

	void set_plugins(std::shared_ptr<Plugins> p) { _plugins = std::move(p); }

	URIs& uris() { return _uris; }

	void message(const Message& msg) override;

	void operator()(const BundleBegin&) {}
	void operator()(const BundleEnd&) {}
	void operator()(const Connect&);
	void operator()(const Copy&);
	void operator()(const Del&);
	void operator()(const Delta&);
	void operator()(const Disconnect&);
	void operator()(const DisconnectAll&);
	void operator()(const Error&) {}
	void operator()(const Get&) {}
	void operator()(const Move&);
	void operator()(const Put&);
	void operator()(const Redo&) {}
	void operator()(const Response&) {}
	void operator()(const SetProperty&);
	void operator()(const Undo&) {}

	INGEN_SIGNAL(new_object, void, std::shared_ptr<ObjectModel>)
	INGEN_SIGNAL(new_plugin, void, std::shared_ptr<PluginModel>)
	INGEN_SIGNAL(plugin_deleted, void, URI)

private:
	std::shared_ptr<ObjectModel> _object(const raul::Path& path);
	std::shared_ptr<PluginModel> _plugin(const URI& uri);
	std::shared_ptr<PluginModel> _plugin(const Atom& uri);
	std::shared_ptr<Resource>    _resource(const URI& uri);

	void add_object(const std::shared_ptr<ObjectModel>& object);
	std::shared_ptr<ObjectModel> remove_object(const raul::Path& path);

	void add_plugin(const std::shared_ptr<PluginModel>& pm);

	std::shared_ptr<GraphModel> connection_graph(const raul::Path& tail_path,
	                                  const raul::Path& head_path);

	// Slots for SigClientInterface signals
	bool attempt_connection(const raul::Path& tail_path,
	                        const raul::Path& head_path);

	URIs&                               _uris;
	Log&                                _log;
	std::shared_ptr<SigClientInterface> _emitter;

	std::shared_ptr<Plugins> _plugins; ///< Map, keyed by plugin URI
};

} // namespace client
} // namespace ingen

#endif // INGEN_CLIENT_CLIENTSTORE_HPP
