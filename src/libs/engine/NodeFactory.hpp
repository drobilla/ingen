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

#ifndef NODEFACTORY_H
#define NODEFACTORY_H

#include CONFIG_H_PATH
#include "module/global.hpp"

#include <list>
#include <map>
#include <string>
#include <ladspa.h>
#include <pthread.h>
#include <glibmm/module.h>
#ifdef HAVE_SLV2
#include <slv2/slv2.h>
#endif
#include "types.hpp"

using std::string;

namespace Ingen {

class NodeImpl;
class PatchImpl;
class PluginImpl;


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
	NodeFactory(Ingen::Shared::World* world);
	~NodeFactory();

	void  load_plugins();
	NodeImpl* load_plugin(PluginImpl* info, const string& name, bool polyphonic, PatchImpl* parent);
	
	typedef std::map<std::string,PluginImpl*> Plugins;
	const Plugins& plugins() const { return _plugins; }
	
	PluginImpl* plugin(const string& uri);

	/** DEPRECATED */
	PluginImpl* plugin(const string& type, const string& lib, const string& label);

private:
#ifdef HAVE_LADSPA
	void load_ladspa_plugins();
	NodeImpl* load_ladspa_plugin(PluginImpl* plugin, const string& name, bool polyphonic, PatchImpl* parent, SampleRate srate, size_t buffer_size);
#endif

#ifdef HAVE_SLV2
	void load_lv2_plugins();
	NodeImpl* load_lv2_plugin(PluginImpl* plugin, const string& name, bool polyphonic, PatchImpl* parent, SampleRate srate, size_t buffer_size);
#endif

	void load_internal_plugins();
	NodeImpl* load_internal_plugin(PluginImpl* plugin, const string& name, bool polyphonic, PatchImpl* parent, SampleRate srate, size_t buffer_size);
	
	Plugins _plugins;

	Ingen::Shared::World* _world;
	bool _has_loaded;
};


} // namespace Ingen

#endif // NODEFACTORY_H
