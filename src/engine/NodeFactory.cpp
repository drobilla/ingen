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

#include "config.h"
#include <cstdlib>
#include <pthread.h>
#include <dirent.h>
#include <float.h>
#include <cmath>
#include <redlandmm/World.hpp>
#include "module/World.hpp"
#include "NodeFactory.hpp"
#include "ThreadManager.hpp"
#include "MidiNoteNode.hpp"
#include "MidiTriggerNode.hpp"
#include "MidiControlNode.hpp"
#include "TransportNode.hpp"
#include "PatchImpl.hpp"
#include "InternalPlugin.hpp"
#ifdef HAVE_LADSPA
#include "LADSPANode.hpp"
#include "LADSPAPlugin.hpp"
#endif
#ifdef HAVE_SLV2
#include <slv2/slv2.h>
#include "LV2Plugin.hpp"
#include "LV2Node.hpp"
#endif

using namespace std;

namespace Ingen {


NodeFactory::NodeFactory(Ingen::Shared::World* world)
	: _world(world)
	, _has_loaded(false)
#ifdef HAVE_SLV2
	, _lv2_info(new LV2Info(world))
#endif
{
}


NodeFactory::~NodeFactory()
{
	for (Plugins::iterator i = _plugins.begin(); i != _plugins.end(); ++i)
		if (i->second->type() != Plugin::Internal)
			delete i->second;

	_plugins.clear();
}


PluginImpl*
NodeFactory::plugin(const string& uri)
{
	const Plugins::const_iterator i = _plugins.find(uri);
	return ((i != _plugins.end()) ? i->second : NULL);
}


/** DEPRECATED: Find a plugin by type, lib, label.
 *
 * Slow.  Evil.  Do not use.
 */
PluginImpl*
NodeFactory::plugin(const string& type, const string& lib, const string& label)
{
	if (type != "LADSPA" || lib == "" || label == "")
		return NULL;

#ifdef HAVE_LADSPA
	for (Plugins::const_iterator i = _plugins.begin(); i != _plugins.end(); ++i) {
		LADSPAPlugin* lp = dynamic_cast<LADSPAPlugin*>(i->second);
		if (lp && lp->type_string() == type
				&& lp->library_name() == lib
				&& lp->label() == label)
			return lp;
	}
#endif

	cerr << "ERROR: Failed to find " << type << " plugin " << lib << " / " << label << endl;

	return NULL;
}


void
NodeFactory::load_plugins()
{
	assert(ThreadManager::current_thread_id() == THREAD_PRE_PROCESS);
			
	_world->rdf_world->mutex().lock();

	// Only load if we havn't already, so every client connecting doesn't cause
	// this (expensive!) stuff to happen.  Not the best solution - would be nice
	// if clients could refresh plugins list for whatever reason :/
	if (!_has_loaded) {
		_plugins.clear(); // FIXME: assert empty?

		load_internal_plugins();
	
#ifdef HAVE_SLV2
		load_lv2_plugins();
#endif

#ifdef HAVE_LADSPA
		load_ladspa_plugins();
#endif
		
		_has_loaded = true;
	}
	
	_world->rdf_world->mutex().unlock();
	
	//cerr << "[NodeFactory] # Plugins: " << _plugins.size() << endl;
}


void
NodeFactory::load_internal_plugins()
{
	// This is a touch gross...
	
	PatchImpl* parent = new PatchImpl(*_world->local_engine, "dummy", 1, NULL, 1, 1, 1);

	NodeImpl* n = NULL;
	n = new MidiNoteNode("foo", 1, parent, 1, 1);
	_plugins.insert(make_pair(n->plugin_impl()->uri(), n->plugin_impl()));
	delete n;
	n = new MidiTriggerNode("foo", 1, parent, 1, 1);
	_plugins.insert(make_pair(n->plugin_impl()->uri(), n->plugin_impl()));
	delete n;
	n = new MidiControlNode("foo", 1, parent, 1, 1);
	_plugins.insert(make_pair(n->plugin_impl()->uri(), n->plugin_impl()));
	delete n;
	n = new TransportNode("foo", 1, parent, 1, 1);
	_plugins.insert(make_pair(n->plugin_impl()->uri(), n->plugin_impl()));
	delete n;
	
	delete parent;
}


#ifdef HAVE_SLV2
/** Loads information about all LV2 plugins into internal plugin database.
 */
void
NodeFactory::load_lv2_plugins()
{
	SLV2Plugins plugins = slv2_world_get_all_plugins(_world->slv2_world);

	//cerr << "[NodeFactory] Found " << slv2_plugins_size(plugins) << " LV2 plugins:" << endl;

	for (unsigned i=0; i < slv2_plugins_size(plugins); ++i) {

		SLV2Plugin lv2_plug = slv2_plugins_get_at(plugins, i);

		const string uri(slv2_value_as_uri(slv2_plugin_get_uri(lv2_plug)));

#ifndef NDEBUG
		assert(_plugins.find(uri) == _plugins.end());
#endif
			
		LV2Plugin* const plugin = new LV2Plugin(_lv2_info, uri);

		plugin->slv2_plugin(lv2_plug);
		plugin->library_path(slv2_uri_to_path(slv2_value_as_uri(
				slv2_plugin_get_library_uri(lv2_plug))));
		_plugins.insert(make_pair(uri, plugin));
	}

	slv2_plugins_free(_world->slv2_world, plugins);
}
#endif // HAVE_SLV2


#ifdef HAVE_LADSPA
/** Loads information about all LADSPA plugins into internal plugin database.
 */
void
NodeFactory::load_ladspa_plugins()
{
	char* env_ladspa_path = getenv("LADSPA_PATH");
	string ladspa_path;
	if (!env_ladspa_path) {
	 	cerr << "[NodeFactory] LADSPA_PATH is empty.  Assuming /usr/lib/ladspa:/usr/local/lib/ladspa:~/.ladspa" << endl;
		ladspa_path = string("/usr/lib/ladspa:/usr/local/lib/ladspa:").append(
			getenv("HOME")).append("/.ladspa");
	} else {
		ladspa_path = env_ladspa_path;
	}

	// Yep, this should use an sstream alright..
	while (ladspa_path != "") {
		const string dir = ladspa_path.substr(0, ladspa_path.find(':'));
		if (ladspa_path.find(':') != string::npos)
			ladspa_path = ladspa_path.substr(ladspa_path.find(':')+1);
		else
			ladspa_path = "";
	
		DIR* pdir = opendir(dir.c_str());
		if (pdir == NULL) {
			//cerr << "[NodeFactory] Unreadable directory in LADSPA_PATH: " << dir.c_str() << endl;
			continue;
		}

		struct dirent* pfile;
		while ((pfile = readdir(pdir))) {
	
			LADSPA_Descriptor_Function df         = NULL;
			LADSPA_Descriptor*         descriptor = NULL;
			
			if (!strcmp(pfile->d_name, ".") || !strcmp(pfile->d_name, ".."))
				continue;
			
			const string lib_path = dir +"/"+ pfile->d_name;
			
			// Ignore stupid libtool files.  Kludge alert.
			if (lib_path.substr(lib_path.length()-3) == ".la") {
				//cerr << "WARNING: Skipping stupid libtool file " << pfile->d_name << endl;
				continue;
			}
			
			Glib::Module* plugin_library = new Glib::Module(lib_path, Glib::MODULE_BIND_LOCAL);
			if (!plugin_library || !(*plugin_library)) {
				cerr << "WARNING: Failed to load LADSPA library " << lib_path << endl;
				continue;
			}
			
			bool found = plugin_library->get_symbol("ladspa_descriptor", (void*&)df);
			if (!found || !df) {
				cerr << "WARNING: Non-LADSPA library found in LADSPA path: " <<
					lib_path << endl;
				// Not a LADSPA plugin library
				delete plugin_library;
				continue;
			}

			for (unsigned long i=0; (descriptor = (LADSPA_Descriptor*)df(i)) != NULL; ++i) {
				char id_str[11];
				snprintf(id_str, 11, "%lu", descriptor->UniqueID);
				const string uri = string("ladspa:").append(id_str);
				
				const Plugins::const_iterator i = _plugins.find(uri);
				
				if (i == _plugins.end()) {
					LADSPAPlugin* plugin = new LADSPAPlugin(lib_path, uri,
						descriptor->UniqueID,
						descriptor->Label,
						descriptor->Name);

					_plugins.insert(make_pair(uri, plugin));

				} else {
					cerr << "Warning: Duplicate " << uri
					     << " - Using " << i->second->library_path()
					     << " over " << lib_path << endl;
				}
			}

			delete plugin_library;
		}
		closedir(pdir);
	}
}
#endif // HAVE_LADSPA


} // namespace Ingen
