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

#ifndef PLUGIN_H
#define PLUGIN_H

#include "config.h"

#include <cstdlib>
#include <boost/utility.hpp>
#include <dlfcn.h>
#include <string>
#include <iostream>
#ifdef HAVE_SLV2
#include <slv2/slv2.h>
#endif
#include "types.h"
using std::string;
using std::cerr; using std::endl;

namespace Ingen {

class PluginLibrary;
class Patch;
class Node;


/** Representation of a plugin (of various types).
 *
 * A Node is an instance of this, conceptually.
 * FIXME: This whole thing is a filthy mess and needs a rewrite.  Probably
 * with derived classes for each plugin type.
 */
class Plugin : boost::noncopyable
{
public:
	enum Type { LV2, LADSPA, DSSI, Internal, Patch };
	
	Plugin(Type type, const string& uri)
	: _type(type)
	, _uri(uri)
	, _id(0)
	, _library(NULL)
#ifdef HAVE_SLV2
	, _slv2_plugin(NULL)
#endif
	{}

	// FIXME: remove
	Plugin() : _type(Internal), _lib_path("/Ingen"),
	           _id(0), _library(NULL)
	{
#ifdef HAVE_SLV2
		_slv2_plugin = NULL;
#endif
	}

	Plugin(const Plugin* const copy) {
		// Copying only allowed for Internal plugins.  Bit of a hack, but
		// allows the PluginInfo to be defined in the Node class which keeps
		// things localized and convenient (FIXME?)
		if (copy->_type != Internal)
			exit(EXIT_FAILURE);
		_type = copy->_type;
		_uri = copy->_uri;
		_lib_path = copy->_lib_path;
		_lib_name = copy->_lib_name;
		_plug_label = copy->_plug_label;
		_name = copy->_name;
		_id = _id;
		_library = copy->_library;
	}
	
	Type          type() const                { return _type; }
	void          type(Type t)                { _type = t; }
	const string& lib_path() const            { return _lib_path; }
	void          lib_path(const string& s)   { _lib_path = s; _lib_name = _lib_path.substr(_lib_path.find_last_of("/")+1); }
	string        lib_name() const            { return _lib_name; }
	void          lib_name(const string& s)   { _lib_name = s; }
	const string& plug_label() const          { return _plug_label; }
	void          plug_label(const string& s) { _plug_label = s; }
	const string& name() const                { return _name; }
	void          name(const string& s)       { _name = s; }
	unsigned long id() const                  { return _id; }
	void          id(unsigned long i)         { _id = i; }
	const string  uri() const                 { return _uri; }
	void          uri(const string& s)        { _uri = s; }
	
	PluginLibrary* library() const             { return _library; }
	void library(PluginLibrary* const library) { _library = library; }
	
	const char* type_string() const {
		if (_type == LADSPA) return "LADSPA";
		else if (_type == LV2) return "LV2";
		else if (_type == DSSI) return "DSSI";
		else if (_type == Internal) return "Internal";
		else if (_type == Patch) return "Patch";
		else return "";
	}
	
	string type_uri() const {
		return string("ingen:") + type_string();
	}

	void set_type(const string& type_string) {
		if (type_string == "LADSPA") _type = LADSPA;
		else if (type_string == "LV2") _type = LV2;
		else if (type_string == "DSSI") _type = DSSI;
		else if (type_string == "Internal") _type = Internal;
		else if (type_string == "Patch") _type = Patch;
	}
	
	// FIXME: ew
#ifdef HAVE_SLV2
	SLV2Plugin* slv2_plugin() const              { return _slv2_plugin; }
	void        slv2_plugin(const SLV2Plugin* p) { _slv2_plugin = p; }
	
#endif

	Node* instantiate(const string& name, size_t poly, Ingen::Patch* parent, SampleRate srate, size_t buffer_size);

private:
	Type   _type;
	string _uri;        ///< LV2 only
	string _lib_path;   ///< LADSPA/DSSI only
	string _lib_name;   ///< LADSPA/DSSI only
	string _plug_label; ///< LADSPA/DSSI only
	string _name;       ///< LADSPA/DSSI only
	unsigned long _id;  ///< LADSPA/DSSI only
	
	PluginLibrary* _library;

#ifdef HAVE_SLV2
	SLV2Plugin* _slv2_plugin;
#endif
};


} // namespace Ingen

#endif // PLUGIN_H

