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
#include "types.hpp"
#include "interface/Plugin.hpp"

using std::string;
using Ingen::Shared::Plugin;

namespace Ingen {

class PatchImpl;
class NodeImpl;


/** Implementation of a plugin (internal code, or a loaded shared library).
 *
 * Conceptually, a Node is an instance of this.
 */
class PluginImpl : public Ingen::Shared::Plugin, public boost::noncopyable
{
public:
	PluginImpl(Type type, const string& uri, const string library_path="")
		: _type(type)
		, _uri(uri)
		, _library_path(library_path)
		, _module(NULL)
	{}
	
	virtual NodeImpl* instantiate(const std::string& name,
	                              bool               polyphonic,
	                              Ingen::PatchImpl*  parent,
	                              SampleRate         srate,
	                              size_t             buffer_size) = 0;
	
	virtual const string symbol() const = 0;
	virtual const string name() const = 0;
	
	const std::string& library_path() const               { return _library_path; }
	void               library_path(const std::string& s) { _library_path = s;}
	
	void load();
	void unload();
	
	const char* type_string() const {
		if (_type == LADSPA) return "LADSPA";
		else if (_type == LV2) return "LV2";
		else if (_type == Internal) return "Internal";
		else if (_type == Patch) return "Patch";
		else return "";
	}
	
	const string type_uri() const {
		return string("ingen:").append(type_string());
	}

	void set_type(const string& type_string) {
		if      (type_string == "LADSPA")   _type = LADSPA;
		else if (type_string == "LV2")      _type = LV2;
		else if (type_string == "Internal") _type = Internal;
		else if (type_string == "Patch")    _type = Patch;
	}
	
	Plugin::Type  type() const                 { return _type; }
	void          type(Plugin::Type t)         { _type = t; }
	const string& uri() const                  { return _uri; }
	Glib::Module* module() const               { return _module; }
	void          module(Glib::Module* module) { _module = module; }

protected:
	Plugin::Type  _type;
	const string  _uri;
	string        _library_path;
	Glib::Module* _module;
};


} // namespace Ingen

#endif // PLUGINIMPL_H

