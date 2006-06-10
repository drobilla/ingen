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
using std::string; using std::map;

namespace LibOmClient { class PatchModel; class PluginModel; class SigClientInterface; }
using namespace LibOmClient;

namespace OmGtk {

class GtkObjectController;
class PatchController;
class NodeController;
class PortController;

/** Singeton which holds all "Om Objects" for easy/fast lookup
 *
 * \ingroup OmGtk
 */
class Store {
public:
	GtkObjectController* object(const string& path) const;
	PatchController*     patch(const string& path) const;
	NodeController*      node(const string& path) const;
	PortController*      port(const string& path) const;

	void add_object(GtkObjectController* object);
	void remove_object(GtkObjectController* object);
	
	size_t num_objects() { return m_objects.size(); }
	
	void add_plugin(const PluginModel* pm);
	const map<string, const PluginModel*>& plugins() const { return m_plugins; }

	static void instantiate(SigClientInterface& emitter)
	{ if (!_instance) _instance = new Store(emitter); }

	inline static Store& instance() { assert(_instance); return *_instance; }

private:
	Store(SigClientInterface& emitter);

	static Store* _instance;

	// Slots for SigClientInterface signals
	void destruction_event(const string& path);
	void new_plugin_event(const string& type, const string& uri, const string& name);

	map<string, GtkObjectController*> m_objects; ///< Keyed by Om path
	map<string, const PluginModel*>   m_plugins; ///< Keyed by URI
};


} // namespace OmGtk

#endif // STORE_H
