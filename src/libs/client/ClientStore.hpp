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

#ifndef CLIENT_STORE_HPP
#define CLIENT_STORE_HPP

#include <cassert>
#include <string>
#include <list>
#include <raul/SharedPtr.hpp>
#include <sigc++/sigc++.h>
#include <raul/Path.hpp>
#include <raul/Atom.hpp>
#include <raul/PathTable.hpp>
#include <raul/TableImpl.hpp>
#include "interface/EngineInterface.hpp"
#include "shared/Store.hpp"
using std::string; using std::list;
using Ingen::Shared::EngineInterface;
using Raul::Path;
using Raul::Atom;

namespace Ingen {

namespace Shared { class GraphObject; }

namespace Client {

class SigClientInterface;
class ObjectModel;
class PluginModel;
class PatchModel;
class NodeModel;
class PortModel;
class ConnectionModel;


/** Automatically manages models of objects in the engine.
 *
 * \ingroup IngenClient
 */
class ClientStore : public Shared::Store, public Shared::CommonInterface, public sigc::trackable {
public:
	ClientStore(SharedPtr<EngineInterface> engine=SharedPtr<EngineInterface>(),
	            SharedPtr<SigClientInterface> emitter=SharedPtr<SigClientInterface>());

	SharedPtr<PluginModel> plugin(const string& uri);
	SharedPtr<ObjectModel> object(const Path& path);

	void clear();

	typedef Raul::Table<string, SharedPtr<PluginModel> > Plugins;
	SharedPtr<const Plugins> plugins() const                   { return _plugins; }
	SharedPtr<Plugins>       plugins()                         { return _plugins; }
	void                     set_plugins(SharedPtr<Plugins> p) { _plugins = p; }
	
	// CommonInterface
	void new_plugin(const string& uri, const string& type_uri, const string& symbol, const string& name);
	void new_patch(const string& path, uint32_t poly);
	void new_node(const string& path, const string& plugin_uri);
	void new_port(const string& path, uint32_t index, const string& data_type, bool is_output);
	void set_variable(const string& subject_path, const string& predicate, const Atom& value);
	void set_property(const string& subject_path, const string& predicate, const Atom& value);
	void set_port_value(const string& port_path, const Raul::Atom& value);
	void set_voice_value(const string& port_path, uint32_t voice, const Raul::Atom& value);
	void connect(const string& src_port_path, const string& dst_port_path);
	void disconnect(const string& src_port_path, const string& dst_port_path);
	void destroy(const string& path);
	
	typedef list< std::pair<Path, Path> > ConnectionRecords;
	const ConnectionRecords& connection_records() { return _connection_orphans; }

	sigc::signal<void, SharedPtr<ObjectModel> > signal_new_object; 
	sigc::signal<void, SharedPtr<PluginModel> > signal_new_plugin; 

private:

	void add(Shared::GraphObject* o) { throw; }

	void add_object(SharedPtr<ObjectModel> object);
	SharedPtr<ObjectModel> remove_object(const Path& path);
	
	void add_plugin(SharedPtr<PluginModel> plugin);

	SharedPtr<PatchModel> connection_patch(const Path& src_port_path, const Path& dst_port_path);

	// It would be nice to integrate these somehow..
	
	void add_orphan(SharedPtr<ObjectModel> orphan);
	void resolve_orphans(SharedPtr<ObjectModel> parent);
	
	void add_connection_orphan(std::pair<Path, Path> orphan);
	void resolve_connection_orphans(SharedPtr<PortModel> port);
	
	void add_plugin_orphan(SharedPtr<NodeModel> orphan);
	void resolve_plugin_orphans(SharedPtr<PluginModel> plugin);
	
	void add_variable_orphan(const Path& subject, const string& predicate, const Atom& value);
	void resolve_variable_orphans(SharedPtr<ObjectModel> subject);
	
	void bundle_begin() {}
	void bundle_end()   {}

	// Slots for SigClientInterface signals
	void rename(const Path& old_path, const Path& new_path);
	void patch_cleared(const Path& path);
	void port_activity(const Path& port_path);
	
	bool attempt_connection(const Path& src_port_path, const Path& dst_port_path, bool add_orphan=false);
	
	bool _handle_orphans;

	SharedPtr<EngineInterface>    _engine;
	SharedPtr<SigClientInterface> _emitter;

	SharedPtr<Plugins> _plugins; ///< Map, keyed by plugin URI

	/** Objects we've received, but depend on the existance of another unknown object.
	 * Keyed by the path of the depended-on object (for tolerance of orderless comms) */
	Raul::PathTable<list<SharedPtr<ObjectModel> > > _orphans;
	
	/** Same idea, except with plugins instead of parents.
	 * It's unfortunate everything doesn't just have a URI and this was the same.. ahem.. */
	Raul::Table<string, list<SharedPtr<NodeModel> > > _plugin_orphans;
	
	/** Not orphans OF variable like the above, but orphans which are variable */
	Raul::PathTable<list<std::pair<string, Atom> > > _variable_orphans;
	
	/** Ditto */
	ConnectionRecords _connection_orphans;
};


} // namespace Client
} // namespace Ingen

#endif // CLIENT_STORE_HPP
