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

#include "NodeFactory.h"
#include "config.h"
#include <cstdlib>
#include <pthread.h>
#include <dirent.h>
#include <float.h>
#include <cmath>
#include "ThreadManager.h"
#include "MidiNoteNode.h"
#include "MidiTriggerNode.h"
#include "MidiControlNode.h"
#include "TransportNode.h"
#include "Plugin.h"
#include "Patch.h"
#ifdef HAVE_SLV2
#include "LV2Node.h"
#include <slv2/slv2.h>
#include <slv2/util.h> // old slv2 compat
#endif
#ifdef HAVE_LADSPA
#include "LADSPANode.h"
#endif
#ifdef HAVE_DSSI
#include "DSSINode.h"
#endif

using std::string;
using std::cerr; using std::cout; using std::endl;


namespace Ingen {


/* I am perfectly aware that the vast majority of this class is a 
 * vomit inducing nightmare at the moment ;)
 */



NodeFactory::NodeFactory()
: _has_loaded(false)
{
#ifdef HAVE_SLV2
	_world = slv2_world_new();
	slv2_world_load_all(_world);
#endif

	// Add builtin plugin types to _internal_plugins list
	// FIXME: ewwww, definitely a better way to do this!

	Patch* parent = new Patch("dummy", 1, NULL, 1, 1, 1);

	Node* n = NULL;
	n = new MidiNoteNode("foo", 1, parent, 1, 1);
	_internal_plugins.push_back(new Plugin(n->plugin()));
	delete n;
	n = new MidiTriggerNode("foo", 1, parent, 1, 1);
	_internal_plugins.push_back(new Plugin(n->plugin()));
	delete n;
	n = new MidiControlNode("foo", 1, parent, 1, 1);
	_internal_plugins.push_back(new Plugin(n->plugin()));
	delete n;
	n = new TransportNode("foo", 1, parent, 1, 1);
	_internal_plugins.push_back(new Plugin(n->plugin()));
	delete n;
	
	delete parent;
}


NodeFactory::~NodeFactory()
{
	for (list<Plugin*>::iterator i = _plugins.begin(); i != _plugins.end(); ++i)
		delete (*i);
	_plugins.clear();

	
	for (list<Glib::Module*>::iterator i = _libraries.begin(); i != _libraries.end(); ++i)
		delete (*i);
	_libraries.clear();

#ifdef HAVE_SLV2
	slv2_world_free(_world);
#endif

}


const Plugin*
NodeFactory::plugin(const string& uri)
{
	for (list<Plugin*>::iterator i = _plugins.begin(); i != _plugins.end(); ++i)
		if ((*i)->uri() == uri)
			return (*i);

	return NULL;
}


/** DEPRECATED: Find a plugin by type, lib, label.
 *
 * Do not use.
 */
const Plugin*
NodeFactory::plugin(const string& type, const string& lib, const string& label)
{
	if (type == "" || lib == "" || label == "")
		return NULL;

	for (list<Plugin*>::iterator i = _plugins.begin(); i != _plugins.end(); ++i)
		if ((*i)->type_string() == type && (*i)->lib_name() == lib && (*i)->plug_label() == label)
			return (*i);

	return NULL;
}


void
NodeFactory::load_plugins()
{
	assert(ThreadManager::current_thread_id() == THREAD_PRE_PROCESS);

	// Only load if we havn't already, so every client connecting doesn't cause
	// this (expensive!) stuff to happen.  Not the best solution - would be nice
	// if clients could refresh plugins list for whatever reason :/
	if (!_has_loaded) {
		_plugins.clear();
		_plugins = _internal_plugins;
	
#ifdef HAVE_SLV2
		load_lv2_plugins();
#endif
#ifdef HAVE_DSSI
		load_dssi_plugins();
#endif
#ifdef HAVE_LADSPA
		load_ladspa_plugins();
#endif
		
		_has_loaded = true;
	}

	//cerr << "[NodeFactory] # Plugins: " << _plugins.size() << endl;
}


/** Loads a plugin.
 *
 * Calls the load_*_plugin functions to actually do things, just a wrapper.
 */
Node*
NodeFactory::load_plugin(const Plugin* a_plugin,
                         const string& name,
                         size_t        poly,
                         Patch*        parent)
{
	assert(parent != NULL);
	assert(poly == 1 || poly == parent->internal_poly());
	assert(a_plugin);

	Node* r = NULL;
	Plugin* plugin = NULL;

	const SampleRate srate       = parent->sample_rate();
	const size_t     buffer_size = parent->buffer_size();

	// FIXME FIXME FIXME: double lookup
	
	// Attempt to find the plugin in loaded DB
	if (a_plugin->type() != Plugin::Internal) {

		// DEPRECATED: Search by lib name / plug label
		if (a_plugin->uri().length() == 0) {
			assert(a_plugin->lib_name().length() > 0 && a_plugin->plug_label().length() > 0);
			//cerr << "Searching for: " << a_plugin->lib_name() << " : " << a_plugin->plug_label() << endl;
			for (list<Plugin*>::iterator i = _plugins.begin(); i != _plugins.end(); ++i) {
				//cerr << (*i)->lib_name() << " : " << (*i)->plug_label() << endl;
				if (a_plugin->lib_name() == (*i)->lib_name() && a_plugin->plug_label() == (*i)->plug_label()) {
					plugin = *i;
					break;
				}
			}
		} else {
			// Search by URI
			for (list<Plugin*>::iterator i = _plugins.begin(); i != _plugins.end(); ++i) {
				if (a_plugin->uri() == (*i)->uri()) {
					plugin = *i;
					break;
				}
			}
		}

		if (plugin == NULL) {
			cerr << "DID NOT FIND PLUGIN " << name << endl;
			return NULL;
		}
	}

	switch (a_plugin->type()) {
#ifdef HAVE_SLV2
	case Plugin::LV2:
		r = load_lv2_plugin(plugin->uri(), name, poly, parent, srate, buffer_size);
		break;
#endif
#ifdef HAVE_DSSI
	case Plugin::DSSI:
		r = load_dssi_plugin(plugin->uri(), name, poly, parent, srate, buffer_size);
		break;
#endif
#ifdef HAVE_LADSPA
	case Plugin::LADSPA:
		r = load_ladspa_plugin(plugin->uri(), name, poly, parent, srate, buffer_size);
		break;
#endif
	case Plugin::Internal:
		r = load_internal_plugin(a_plugin->uri(), name, poly, parent, srate, buffer_size);
		break;
	default:
		cerr << "[NodeFactory] WARNING: Unknown plugin type." << endl;
	}

	return r;
}


/** Loads an internal plugin.
 */
Node*
NodeFactory::load_internal_plugin(const string& uri,
                                  const string& name,
                                  size_t        poly,
                                  Patch*        parent,
                                  SampleRate    srate,
                                  size_t        buffer_size)
{
	assert(parent != NULL);
	assert(poly == 1 || poly == parent->internal_poly());
	assert(uri.length() > 6);
	assert(uri.substr(0, 6) == "ingen:");

	for (list<Plugin*>::iterator i = _internal_plugins.begin(); i != _internal_plugins.end(); ++i)
		if ((*i)->uri() == uri)
			return (*i)->instantiate(name, poly, parent, srate, buffer_size);
	
	return NULL;
}


#ifdef HAVE_SLV2

/** Loads information about all LV2 plugins into internal plugin database.
 */
void
NodeFactory::load_lv2_plugins()
{
	SLV2Plugins plugins = slv2_world_get_all_plugins(_world);

	//cerr << "[NodeFactory] Found " << slv2_plugins_size(plugins) << " LV2 plugins:" << endl;

	for (unsigned i=0; i < slv2_plugins_size(plugins); ++i) {

		SLV2Plugin lv2_plug = slv2_plugins_get_at(plugins, i);

		const char* uri = (const char*)slv2_plugin_get_uri(lv2_plug);
		assert(uri);
		//cerr << "\t" << uri << endl;

		Plugin* plug = NULL;

		bool found = false;
		for (list<Plugin*>::const_iterator i = _plugins.begin(); i != _plugins.end(); ++i) {
			if (!strcmp((*i)->uri().c_str(), uri)) {
				plug = (*i);
				found = true;
				break;
			}
		}

		if (!found)
			plug = new Plugin(Plugin::LV2, uri);

		plug->slv2_plugin(lv2_plug);
		plug->module(NULL); // FIXME?
		plug->lib_path(slv2_uri_to_path(slv2_plugin_get_library_uri(lv2_plug)));
		char* name = slv2_plugin_get_name(lv2_plug);
		assert(name);
		plug->name(name);
		free(name);
		
		if (!found)
			_plugins.push_back(plug);
	}

	slv2_plugins_free(_world, plugins);
}


/** Loads a LV2 plugin.
 * Returns 'poly' independant plugins as a Node*
 */
Node*
NodeFactory::load_lv2_plugin(const string& plug_uri,
                             const string& node_name,
                             size_t        poly,
                             Patch*        parent,
                             SampleRate    srate,
                             size_t        buffer_size)
{
	// Find (internal) Plugin
	Plugin* plugin = NULL;
	list<Plugin*>::iterator i;
	for (i = _plugins.begin(); i != _plugins.end(); ++i) {
		plugin = (*i);
		if ((*i)->uri() == plug_uri) break;
	}
	
	Node* n = NULL;

	if (plugin) {
		n = new LV2Node(plugin, node_name, poly, parent, srate, buffer_size);
		bool success = ((LV2Node*)n)->instantiate();
		if (!success) {
			delete n;
			n = NULL;
		}
	}
	
	return n;
}

#endif // HAVE_SLV2


#ifdef HAVE_DSSI

/** Loads information about all DSSI plugins into internal plugin database.
 */
void
NodeFactory::load_dssi_plugins()
{
	// FIXME: too much code duplication with load_ladspa_plugin
	
	char* env_dssi_path = getenv("DSSI_PATH");
	string dssi_path;
	if (!env_dssi_path) {
	 	cerr << "[NodeFactory] DSSI_PATH is empty.  Assuming /usr/lib/dssi:/usr/local/lib/dssi:~/.dssi" << endl;
		dssi_path = string("/usr/lib/dssi:/usr/local/lib/dssi:").append(
			getenv("HOME")).append("/.dssi");
	} else {
		dssi_path = env_dssi_path;
	}

	DSSI_Descriptor_Function df         = NULL;
	DSSI_Descriptor*         descriptor = NULL;
	
	string dir;
	string full_lib_name;
	
	// Yep, this should use an sstream alright..
	while (dssi_path != "") {
		dir = dssi_path.substr(0, dssi_path.find(':'));
		if (dssi_path.find(':') != string::npos)
			dssi_path = dssi_path.substr(dssi_path.find(':')+1);
		else
			dssi_path = "";
	
		DIR* pdir = opendir(dir.c_str());
		if (pdir == NULL) {
			//cerr << "[NodeFactory] Unreadable directory in DSSI_PATH: " << dir.c_str() << endl;
			continue;
		}

		struct dirent* pfile;
		while ((pfile = readdir(pdir))) {
			
			if (!strcmp(pfile->d_name, ".") || !strcmp(pfile->d_name, ".."))
				continue;
			
			full_lib_name = dir +"/"+ pfile->d_name;
			
			// Load descriptor function
			// Loaded with LAZY here, will be loaded with NOW on node loading
			void* handle = dlopen(full_lib_name.c_str(), RTLD_LAZY);
			if (handle == NULL)
				continue;
			
			df = (DSSI_Descriptor_Function)dlsym(handle, "dssi_descriptor");
			if (df == NULL) {
				// Not a DSSI plugin library
				dlclose(handle);
				continue;
			}

			Glib::Module* plugin_library = new Glib::Module(full_lib_name);
			if (!(*plugin_library))
				continue;

			_libraries.push_back(plugin_library);

			const LADSPA_Descriptor* ld = NULL;
			
			for (unsigned long i=0; (descriptor = (DSSI_Descriptor*)df(i)) != NULL; ++i) {
				ld = descriptor->LADSPA_Plugin;
				assert(ld != NULL);
				string uri = string("dssi:") + pfile->d_name +":"+ ld->Label;
				Plugin* plugin = new Plugin(Plugin::DSSI, uri);
				plugin->module(plugin_library);
				plugin->lib_path(dir + "/" + pfile->d_name);
				plugin->plug_label(ld->Label);
				plugin->name(ld->Name);
				plugin->type(Plugin::DSSI);
				plugin->id(ld->UniqueID);

				bool found = false;
				for (list<Plugin*>::const_iterator i = _plugins.begin(); i != _plugins.end(); ++i) {
					if ((*i)->uri() == plugin->uri()) {
						cerr << "Warning: Duplicate DSSI plugin (" << plugin->lib_name() << ":"
							<< plugin->plug_label() << ")" << " found.\nUsing " << (*i)->lib_path()
							<< " version." << endl;
						found = true;
						break;
					}
				}
				if (!found)
					_plugins.push_back(plugin);
				else
					delete plugin;
			}
			
			df = NULL;
			descriptor = NULL;
			dlclose(handle);
		}
		closedir(pdir);
	}
}


/** Creates a Node by instancing a DSSI plugin.
 */
Node*
NodeFactory::load_dssi_plugin(const string& uri,
                              const string& name, size_t poly, Patch* parent, SampleRate srate, size_t buffer_size)
{
	// FIXME: awful code duplication here
	
	assert(uri != "");
	assert(name != "");
	assert(poly > 0);
	
	DSSI_Descriptor_Function df = NULL;
	const Plugin* plugin = NULL;
	Node* n = NULL;
	
	// Attempt to find the lib
	list<Plugin*>::iterator i;
	for (i = _plugins.begin(); i != _plugins.end(); ++i) {
		plugin = (*i);
		if (plugin->uri() == uri) break;
	}

	assert(plugin->id() != 0);

	if (i == _plugins.end()) {
		cerr << "Did not find DSSI plugin " << uri << " in database." << endl;
		return NULL;
	} else {
		assert(plugin);
		if (!plugin->module()->get_symbol("dssi_descriptor", (void*&)df)) {
			cerr << "Looks like this isn't a DSSI plugin." << endl;
			return NULL;
		}
	}

	// Attempt to find the plugin in lib
	DSSI_Descriptor* descriptor = NULL;
	for (unsigned long i=0; (descriptor = (DSSI_Descriptor*)df(i)) != NULL; ++i) {
		if (descriptor->LADSPA_Plugin != NULL
				&& descriptor->LADSPA_Plugin->UniqueID == plugin->id()) {
			break;
		}
	}
	
	if (descriptor == NULL) {
		cerr << "Could not find plugin \"" << plugin->id() << "\" in " << plugin->lib_name() << endl;
		return NULL;
	}

	n = new DSSINode(plugin, name, poly, parent, descriptor, srate, buffer_size);

	bool success = ((DSSINode*)n)->instantiate();
	if (!success) {
		delete n;
		n = NULL;
	}

	return n;
}
#endif // HAVE_DSSI


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

	LADSPA_Descriptor_Function df         = NULL;
	LADSPA_Descriptor*         descriptor = NULL;
	
	string dir;
	string full_lib_name;
	
	// Yep, this should use an sstream alright..
	while (ladspa_path != "") {
		dir = ladspa_path.substr(0, ladspa_path.find(':'));
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
			
			if (!strcmp(pfile->d_name, ".") || !strcmp(pfile->d_name, ".."))
				continue;
			
			full_lib_name = dir +"/"+ pfile->d_name;
			
			// Load descriptor function
			// Loaded with LAZY here, will be loaded with NOW on node loading
			void* handle = dlopen(full_lib_name.c_str(), RTLD_LAZY);
			if (handle == NULL)
				continue;
			
			df = (LADSPA_Descriptor_Function)dlsym(handle, "ladspa_descriptor");
			if (df == NULL) {
				dlclose(handle);
				continue;
			}	

			Glib::Module* plugin_library = new Glib::Module(full_lib_name);
			if (!(*plugin_library))
				continue;
			
			_libraries.push_back(plugin_library);

			for (unsigned long i=0; (descriptor = (LADSPA_Descriptor*)df(i)) != NULL; ++i) {
				char id_str[11];
				snprintf(id_str, 11, "%lu", descriptor->UniqueID);
				string uri = string("ladspa:").append(id_str);
				Plugin* plugin = new Plugin(Plugin::LADSPA, uri);
				
				assert(plugin_library != NULL);
				plugin->module(plugin_library);
				plugin->lib_path(dir + "/" + pfile->d_name);
				plugin->plug_label(descriptor->Label);
				plugin->name(descriptor->Name);
				plugin->type(Plugin::LADSPA);
				plugin->id(descriptor->UniqueID);
				
				bool found = false;
				for (list<Plugin*>::const_iterator i = _plugins.begin(); i != _plugins.end(); ++i) {
					if ((*i)->uri() == plugin->uri()) {
						cerr << "Warning: Duplicate LADSPA plugin " << plugin->uri()
							<< " found.\nChoosing " << (*i)->lib_path()
							<< " over " << plugin->lib_path() << endl;
						found = true;
						break;
					}
				}
				if (!found)
					_plugins.push_back(plugin);
				else
					delete plugin;
			}
			
			df = NULL;
			descriptor = NULL;
			dlclose(handle);
		}
		closedir(pdir);
	}
}


/** Loads a LADSPA plugin.
 * Returns 'poly' independant plugins as a Node*
 */
Node*
NodeFactory::load_ladspa_plugin(const string& uri,
                                const string& name,
                                size_t        poly,
                                Patch*        parent,
                                SampleRate    srate,
                                size_t        buffer_size)
{
	assert(uri != "");
	assert(name != "");
	assert(poly > 0);
	
	LADSPA_Descriptor_Function df = NULL;
	Plugin* plugin = NULL;
	Node* n = NULL;
	
	// Attempt to find the lib
	list<Plugin*>::iterator i;
	for (i = _plugins.begin(); i != _plugins.end(); ++i) {
		plugin = (*i);
		if (plugin->uri() == uri) break;
	}

	assert(plugin->id() != 0);

	if (i == _plugins.end()) {
		cerr << "Did not find LADSPA plugin " << uri << " in database." << endl;
		return NULL;
	} else {
		if (!plugin->module()->get_symbol("ladspa_descriptor", (void*&)df)) {
			cerr << "Looks like this isn't a LADSPA plugin." << endl;
			return NULL;
		}
	}

	// Attempt to find the plugin in lib
	LADSPA_Descriptor* descriptor = NULL;
	for (unsigned long i=0; (descriptor = (LADSPA_Descriptor*)df(i)) != NULL; ++i) {
		if (descriptor->UniqueID == plugin->id()) {
			break;
		}
	}
	
	if (descriptor == NULL) {
		cerr << "Could not find plugin \"" << plugin->id() << "\" in " << plugin->lib_path() << endl;
		return NULL;
	}

	n = new LADSPANode(plugin, name, poly, parent, descriptor, srate, buffer_size);

	bool success = ((LADSPANode*)n)->instantiate();
	if (!success) {
		delete n;
		n = NULL;
	}
	
	return n;
}


#endif // HAVE_LADSPA


} // namespace Ingen
