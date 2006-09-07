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


#ifndef NODEFACTORY_H
#define NODEFACTORY_H

#include "config.h"
#include "types.h"
#include <list>
#include <string>
#include <ladspa.h>
#include <pthread.h>

using std::string; using std::list;

namespace Ingen {

class Node;
class Patch;
class PluginLibrary;
class Plugin;


/** Loads plugins and creates Nodes from them.
 *
 * NodeFactory's responsibility is to get enough information to allow the 
 * loading of a plugin possible (ie finding/opening shared libraries etc)
 *
 * The constructor of various Node types (ie LADSPANode) are responsible
 * for actually creating a Node instance of the plugin.
 *
 * \ingroup engine
 */
class NodeFactory
{
public:
	NodeFactory();
	~NodeFactory();

	void  load_plugins();
	Node* load_plugin(const Plugin* info, const string& name, size_t poly, Patch* parent);
	
	const list<Plugin*>& plugins() { return _plugins; }
	
	void lock_plugin_list()   { pthread_mutex_lock(&_plugin_list_mutex); }
	void unlock_plugin_list() { pthread_mutex_unlock(&_plugin_list_mutex); }
	
private:
#ifdef HAVE_LADSPA
	void load_ladspa_plugins();
	Node* load_ladspa_plugin(const string& plugin_uri, const string& name, size_t poly, Patch* parent, SampleRate srate, size_t buffer_size);
#endif

#ifdef HAVE_SLV2
	void load_lv2_plugins();
	Node* load_lv2_plugin(const string& plugin_uri, const string& name, size_t poly, Patch* parent, SampleRate srate, size_t buffer_size);
#endif

#ifdef HAVE_DSSI
	void load_dssi_plugins();
	Node* load_dssi_plugin(const string& plugin_uri, const string& name, size_t poly, Patch* parent, SampleRate srate, size_t buffer_size);
#endif
	
	Node* load_internal_plugin(const string& plug_label, const string& name, size_t poly, Patch* parent, SampleRate srate, size_t buffer_size);
	
	list<PluginLibrary*> _libraries;
	list<Plugin*>        _internal_plugins;
	list<Plugin*>        _plugins;

	/** Used to protect the list while load_plugins is building it. */
	pthread_mutex_t _plugin_list_mutex;

	bool _has_loaded;
};


} // namespace Ingen

#endif // NODEFACTORY_H
