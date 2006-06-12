/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef STORE_H
#define STORE_H

#include <cassert>
#include <string>
#include <map>
#include "util/CountedPtr.h"
using std::string; using std::map;

namespace LibOmClient {

class SigClientInterface;
class ObjectModel;
class PluginModel;
class PatchModel;
class NodeModel;
class PortModel;

/** Singeton which holds all "Om Objects" for easy/fast lookup
 *
 * \ingroup OmGtk
 */
class Store {
public:
	CountedPtr<PluginModel> plugin(const string& uri);
	CountedPtr<ObjectModel> object(const string& path);
	CountedPtr<PatchModel>  patch(const string& path);
	CountedPtr<NodeModel>   node(const string& path);
	CountedPtr<PortModel>   port(const string& path);

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
	void metadata_update_event(const string& subject_path, const string& predicate, const string& value);
	void connection_event(const string& src_port_path, const string& dst_port_path);
	void disconnection_event(const string& src_port_path, const string& dst_port_path);
	
	map<string, CountedPtr<ObjectModel> > m_objects; ///< Keyed by Om path
	map<string, CountedPtr<PluginModel> > m_plugins; ///< Keyed by URI
};


} // namespace LibOmClient

#endif // STORE_H
