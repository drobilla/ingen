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

#ifndef STORE_H
#define STORE_H

#include <cassert>
#include <string>
#include <list>
#include <raul/SharedPtr.hpp>
#include <sigc++/sigc++.h>
#include <raul/Path.hpp>
#include <raul/Atom.hpp>
#include <raul/Table.hpp>
#include <raul/TableImpl.hpp>
#include "interface/EngineInterface.hpp"
using std::string; using std::list;
using Ingen::Shared::EngineInterface;
using Raul::Path;
using Raul::Atom;

namespace Ingen {
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
class Store : public sigc::trackable { // FIXME: is trackable necessary?
public:
	Store(SharedPtr<EngineInterface> engine, SharedPtr<SigClientInterface> emitter);

	SharedPtr<PluginModel> plugin(const string& uri);
	SharedPtr<ObjectModel> object(const Path& path);

	void clear();

	size_t num_object() { return _objects.size(); }
	
	typedef Raul::Table<string, SharedPtr<PluginModel> > Plugins;
	const Plugins& plugins() const { return _plugins; }

	typedef Raul::Table<Path, SharedPtr<ObjectModel> > Objects;
	const Objects& objects() const { return _objects; }

	sigc::signal<void, SharedPtr<ObjectModel> > new_object_sig; 
private:

	void add_object(SharedPtr<ObjectModel> object);
	SharedPtr<ObjectModel> remove_object(const Path& path);
	
	void add_plugin(SharedPtr<PluginModel> plugin);

	SharedPtr<PatchModel> connection_patch(const Path& src_port_path, const Path& dst_port_path);

	// It would be nice to integrate these somehow..
	
	void add_orphan(SharedPtr<ObjectModel> orphan);
	void resolve_orphans(SharedPtr<ObjectModel> parent);
	
	void add_connection_orphan(SharedPtr<ConnectionModel> orphan);
	void resolve_connection_orphans(SharedPtr<PortModel> port);
	
	void add_plugin_orphan(SharedPtr<NodeModel> orphan);
	void resolve_plugin_orphans(SharedPtr<PluginModel> plugin);
	
	void add_metadata_orphan(const Path& subject, const string& predicate, const Atom& value);
	void resolve_metadata_orphans(SharedPtr<ObjectModel> subject);

	// Slots for SigClientInterface signals
	void destruction_event(const Path& path);
	void rename_event(const Path& old_path, const Path& new_path);
	void new_plugin_event(const string& uri, const string& type_uri, const string& name);
	void new_patch_event(const Path& path, uint32_t poly);
	void new_node_event(const string& plugin_uri, const Path& node_path, bool is_polyphonic, uint32_t num_ports);
	void new_port_event(const Path& path, const string& data_type, bool is_output);
	void patch_enabled_event(const Path& path);
	void patch_disabled_event(const Path& path);
	void patch_cleared_event(const Path& path);
	void metadata_update_event(const Path& subject_path, const string& predicate, const Atom& value);
	void control_change_event(const Path& port_path, float value);
	void connection_event(const Path& src_port_path, const Path& dst_port_path);
	void disconnection_event(const Path& src_port_path, const Path& dst_port_path);
	
	SharedPtr<EngineInterface>    _engine;
	SharedPtr<SigClientInterface> _emitter;

	Objects _objects; ///< Map, keyed by Ingen path
	Plugins _plugins; ///< Map, keyed by plugin URI

	/** Objects we've received, but depend on the existance of another unknown object.
	 * Keyed by the path of the depended-on object (for tolerance of orderless comms) */
	Raul::Table<Path, list<SharedPtr<ObjectModel> > > _orphans;
	
	/** Same idea, except with plugins instead of parents.
	 * It's unfortunate everything doesn't just have a URI and this was the same.. ahem.. */
	Raul::Table<string, list<SharedPtr<NodeModel> > > _plugin_orphans;
	
	/** Not orphans OF metadata like the above, but orphans which are metadata */
	Raul::Table<Path, list<std::pair<string, Atom> > > _metadata_orphans;
	
	/** Ditto */
	list<SharedPtr<ConnectionModel> > _connection_orphans;
};


} // namespace Client
} // namespace Ingen

#endif // STORE_H
