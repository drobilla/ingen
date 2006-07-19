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

#ifndef PLUGIN_H
#define PLUGIN_H

#include "config.h"

#include <cstdlib>
#include <dlfcn.h>
#include <string>
#include <iostream>
#ifdef HAVE_SLV2
#include <slv2/slv2.h>
#endif
using std::string;
using std::cerr; using std::endl;


namespace Ingen {

class PluginLibrary;


/** Representation of a plugin (of various types).
 *
 * A Node is an instance of this, conceptually.
 * FIXME: This whole thing is a filthy mess and needs a rewrite.  Probably
 * with derived classes for each plugin type.
 */
class Plugin
{
public:
	enum Type { LV2, LADSPA, DSSI, Internal, Patch };
	
	Plugin(Type type, const string& uri)
	: m_type(type)
	, m_uri(uri)
	{}

	// FIXME: remove
	Plugin() : m_type(Internal), m_lib_path("/Ingen"),
	           m_id(0), m_library(NULL)
	{
#ifdef HAVE_SLV2
		m_slv2_plugin = NULL;
#endif
	}

	Plugin(const Plugin* const copy) {
		// Copying only allowed for Internal plugins.  Bit of a hack, but
		// allows the PluginInfo to be defined in the Node class which keeps
		// things localized and convenient (FIXME?)
		if (copy->m_type != Internal)
			exit(EXIT_FAILURE);
		m_type = copy->m_type;
		m_lib_path = copy->m_lib_path;
		m_plug_label = copy->m_plug_label;
		m_name = copy->m_name;
		m_library = copy->m_library;
	}
	
	Type          type() const                { return m_type; }
	void          type(Type t)                { m_type = t; }
	const string& lib_path() const            { return m_lib_path; }
	void          lib_path(const string& s)   { m_lib_path = s; m_lib_name = m_lib_path.substr(m_lib_path.find_last_of("/")+1); }
	string        lib_name() const            { return m_lib_name; }
	void          lib_name(const string& s)   { m_lib_name = s; }
	const string& plug_label() const          { return m_plug_label; }
	void          plug_label(const string& s) { m_plug_label = s; }
	const string& name() const                { return m_name; }
	void          name(const string& s)       { m_name = s; }
	unsigned long id() const                  { return m_id; }
	void          id(unsigned long i)         { m_id = i; }
	const string  uri() const                 { return m_uri; }
	void          uri(const string& s)        { m_uri = s; }
	
	PluginLibrary* library() const             { return m_library; }
	void library(PluginLibrary* const library) { m_library = library; }
	
	const char* type_string() const {
		if (m_type == LADSPA) return "LADSPA";
		else if (m_type == LV2) return "LV2";
		else if (m_type == DSSI) return "DSSI";
		else if (m_type == Internal) return "Internal";
		else if (m_type == Patch) return "Patch";
		else return "";
	}

	void set_type(const string& type_string) {
		if (type_string == "LADSPA") m_type = LADSPA;
		else if (type_string == "LV2") m_type = LV2;
		else if (type_string == "DSSI") m_type = DSSI;
		else if (type_string == "Internal") m_type = Internal;
		else if (type_string == "Patch") m_type = Patch;
	}
	
	// FIXME: ew
#ifdef HAVE_SLV2
	SLV2Plugin* slv2_plugin() const              { return m_slv2_plugin; }
	void        slv2_plugin(const SLV2Plugin* p) { m_slv2_plugin = p; }
	
#endif
private:
	// Disallow copies (undefined)
	Plugin(const Plugin&);
	Plugin& operator=(const Plugin&);
	
	Type   m_type;
	string m_uri;        ///< LV2 only
	string m_lib_path;   ///< LADSPA/DSSI only
	string m_lib_name;   ///< LADSPA/DSSI only
	string m_plug_label; ///< LADSPA/DSSI only
	string m_name;       ///< LADSPA/DSSI only
	unsigned long m_id;  ///< LADSPA/DSSI only
	
	PluginLibrary* m_library;

#ifdef HAVE_SLV2
	SLV2Plugin* m_slv2_plugin;
#endif
};


} // namespace Ingen

#endif // PLUGIN_H

