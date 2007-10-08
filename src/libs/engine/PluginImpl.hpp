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

#ifndef PLUGINIMPL_H
#define PLUGINIMPL_H

#include CONFIG_H_PATH

#include <cstdlib>
#include <glibmm/module.h>
#include <boost/utility.hpp>
#include <dlfcn.h>
#include <string>
#include <iostream>
#ifdef HAVE_SLV2
#include <slv2/slv2.h>
#endif
#include "types.hpp"
#include "interface/Plugin.hpp"
using std::string;
using Ingen::Shared::Plugin;

namespace Ingen {

class PatchImpl;
class NodeImpl;


/** Representation of a plugin (of various types).
 *
 * A Node is an instance of this, conceptually.
 * FIXME: This whole thing is a filthy mess and needs a rewrite.  Probably
 * with derived classes for each plugin type.
 */
class PluginImpl : public Ingen::Shared::Plugin, public boost::noncopyable
{
public:
	PluginImpl(Type type, const string& uri)
	: _type(type)
	, _uri(uri)
	, _id(0)
	, _module(NULL)
#ifdef HAVE_SLV2
	, _slv2_plugin(NULL)
#endif
	{}

	PluginImpl(const PluginImpl* const copy) {
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
		_module = copy->_module;
	}
	
	Plugin::Type  type() const                 { return _type; }
	void          type(Plugin::Type t)         { _type = t; }
	const string& lib_path() const             { return _lib_path; }
	void          lib_path(const string& s)    { _lib_path = s; _lib_name = _lib_path.substr(_lib_path.find_last_of("/")+1); }
	string        lib_name() const             { return _lib_name; }
	void          lib_name(const string& s)    { _lib_name = s; }
	const string& plug_label() const           { return _plug_label; }
	void          plug_label(const string& s)  { _plug_label = s; }
	const string& name() const                 { return _name; }
	void          name(const string& s)        { _name = s; }
	unsigned long id() const                   { return _id; }
	void          id(unsigned long i)          { _id = i; }
	const string& uri() const                  { return _uri; }
	void          uri(const string& s)         { _uri = s; }
	Glib::Module* module() const               { return _module; }
	void          module(Glib::Module* module) { _module = module; }
	
	const char* type_string() const {
		if (_type == LADSPA) return "LADSPA";
		else if (_type == LV2) return "LV2";
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
		else if (type_string == "Internal") _type = Internal;
		else if (type_string == "Patch") _type = Patch;
	}
	
	// FIXME: ew
#ifdef HAVE_SLV2
	SLV2Plugin slv2_plugin() const       { return _slv2_plugin; }
	void       slv2_plugin(SLV2Plugin p) { _slv2_plugin = p; }	
#endif

	NodeImpl* instantiate(const string& name, bool polyphonic, Ingen::PatchImpl* parent, SampleRate srate, size_t buffer_size);

private:
	Plugin::Type  _type;
	string        _uri;        ///< LV2 only
	string        _lib_path;   ///< LADSPA only
	string        _lib_name;   ///< LADSPA only
	string        _plug_label; ///< LADSPA only
	string        _name;       ///< LADSPA only
	unsigned long _id;         ///< LADSPA only
	
	Glib::Module* _module;

#ifdef HAVE_SLV2
	SLV2Plugin _slv2_plugin;
#endif
};


} // namespace Ingen

#endif // PLUGINIMPL_H

