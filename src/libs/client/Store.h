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
#include "util/CountedPtr.h"
#include <sigc++/sigc++.h>
#include "util/Path.h"
using std::string; using std::map;

namespace Ingen {
namespace Client {

class SigClientInterface;
class ObjectModel;
class PluginModel;
class PatchModel;
class NodeModel;
class PortModel;

/** Singeton which holds all "Ingen Objects" for easy/fast lookup
 *
 * \ingroup IngenClient
 */
class Store : public sigc::trackable { // FIXME: is trackable necessary?
public:
	CountedPtr<PluginModel> plugin(const string& uri);
	CountedPtr<ObjectModel> object(const string& path);
	/*CountedPtr<PatchModel>  patch(const string& path);
	CountedPtr<NodeModel>   node(const string& path);
	CountedPtr<PortModel>   port(const string& path);*/

	size_t num_objects() { return m_objects.size(); }
	
	const map<string, CountedPtr<PluginModel> >& plugins() const { return m_plugins; }

	static void instantiate(SigClientInterface& emitter)
	{ if (!_instance) _instance = new Store(emitter); }

	inline static Store& instance() { assert(_instance); return *_instance; }

private:
	Store(SigClientInterface& emitter);

	static Store* _instance;

	void add_object(CountedPtr<ObjectModel> object);
	CountedPtr<ObjectModel> remove_object(const string& path);
	
	void add_plugin(CountedPtr<PluginModel> plugin);

	// Slots for SigClientInterface signals
	void destruction_event(const string& path);
	void new_plugin_event(const string& type, const string& uri, const string& name);
	void new_patch_event(const string& path, uint32_t poly);
	void new_node_event(const string& plugin_type, const string& plugin_uri, const string& node_path, bool is_polyphonic, uint32_t num_ports);
	void new_port_event(const string& path, const string& data_type, bool is_output);
	void patch_enabled_event(const string& path);
	void patch_disabled_event(const string& path);
	void metadata_update_event(const string& subject_path, const string& predicate, const string& value);
	void control_change_event(const string& port_path, float value);
	void connection_event(const Path& src_port_path, const Path& dst_port_path);
	void disconnection_event(const Path& src_port_path, const Path& dst_port_path);
	
	map<string, CountedPtr<ObjectModel> > m_objects; ///< Keyed by Ingen path
	map<string, CountedPtr<PluginModel> > m_plugins; ///< Keyed by URI
};


} // namespace Client
} // namespace Ingen

#endif // STORE_H
