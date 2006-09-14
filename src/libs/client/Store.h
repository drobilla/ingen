/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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
#include <map>
#include <list>
#include "util/CountedPtr.h"
#include <sigc++/sigc++.h>
#include "util/Path.h"
#include "util/Atom.h"
using std::string; using std::map; using std::list;

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
	Store(CountedPtr<SigClientInterface> emitter);

	CountedPtr<PluginModel> plugin(const string& uri);
	CountedPtr<ObjectModel> object(const Path& path);

	void clear();

	size_t num_objects() { return m_objects.size(); }
	
	const map<string, CountedPtr<PluginModel> >& plugins() const { return m_plugins; }

	sigc::signal<void, CountedPtr<ObjectModel> > new_object_sig; 
private:

	void add_object(CountedPtr<ObjectModel> object);
	CountedPtr<ObjectModel> remove_object(const Path& path);
	
	void add_plugin(CountedPtr<PluginModel> plugin);

	// It would be nice to integrate these somehow..
	
	void add_orphan(CountedPtr<ObjectModel> orphan);
	void resolve_orphans(CountedPtr<ObjectModel> parent);
	
	void add_connection_orphan(CountedPtr<ConnectionModel> orphan);
	void resolve_connection_orphans(CountedPtr<PortModel> port);
	
	void add_plugin_orphan(CountedPtr<NodeModel> orphan);
	void resolve_plugin_orphans(CountedPtr<PluginModel> plugin);

	// Slots for SigClientInterface signals
	void destruction_event(const Path& path);
	void new_plugin_event(const string& type, const string& uri, const string& name);
	void new_patch_event(const Path& path, uint32_t poly);
	void new_node_event(const string& plugin_type, const string& plugin_uri, const Path& node_path, bool is_polyphonic, uint32_t num_ports);
	void new_port_event(const Path& path, const string& data_type, bool is_output);
	void patch_enabled_event(const Path& path);
	void patch_disabled_event(const Path& path);
	void metadata_update_event(const Path& subject_path, const string& predicate, const Atom& value);
	void control_change_event(const Path& port_path, float value);
	void connection_event(const Path& src_port_path, const Path& dst_port_path);
	void disconnection_event(const Path& src_port_path, const Path& dst_port_path);
	
	typedef map<Path, CountedPtr<ObjectModel> > ObjectMap;
	ObjectMap m_objects; ///< Keyed by Ingen path

	map<string, CountedPtr<PluginModel> > m_plugins; ///< Keyed by URI

	/** Objects we've received, but depend on the existance of another unknown object.
	 * Keyed by the path of the depended-on object (for tolerance of orderless comms) */
	map<Path, list<CountedPtr<ObjectModel> > > m_orphans;
	
	/** Same idea, except with plugins instead of parents.
	 * It's unfortunate everything doesn't just have a URI and this was the same.. ahem.. */
	map<string, list<CountedPtr<NodeModel> > > m_plugin_orphans;
};


} // namespace Client
} // namespace Ingen

#endif // STORE_H
