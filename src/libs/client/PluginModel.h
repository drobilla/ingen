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

#ifndef PLUGINMODEL_H
#define PLUGINMODEL_H

#include "../../config.h"
#include <string>
#include <iostream>
#include "raul/Path.h"
#ifdef HAVE_SLV2
#include <slv2/slv2.h>
#endif
using std::string; using std::cerr; using std::endl;

namespace Ingen {
namespace Client {


/** Model for a plugin available for loading.
 *
 * \ingroup IngenClient
 */
class PluginModel
{
public:
	enum Type { LV2, LADSPA, DSSI, Internal, Patch };

	PluginModel(const string& uri, const string& type_uri, const string& name)
		: _uri(uri)
		, _name(name)
	{
		set_type_from_uri(type_uri);
#ifdef HAVE_SLV2
		static SLV2Plugins plugins = NULL;
		if (!plugins) {
			plugins = slv2_plugins_new();
			slv2_plugins_load_all(plugins);
		}
		
		_slv2_plugin = slv2_plugins_get_by_uri(plugins, uri.c_str());
#endif
	}
	
	Type          type() const                { return _type; }
	void          type(Type t)                { _type = t; }
	const string& uri() const                 { return _uri; }
	void          uri(const string& s)        { _uri = s; }
	const string& name() const                { return _name; }
	void          name(const string& s)       { _name = s; }
	
	/*const char* const type_string() const {
		if (_type == LV2) return "LV2";
		else if (_type == LADSPA) return "LADSPA";
		else if (_type == DSSI) return "DSSI";
		else if (_type == Internal) return "Internal";
		else if (_type == Patch) return "Patch";
		else return "";
	}*/

	const char* const type_uri() const {
		if (_type == LV2) return "ingen:LV2Plugin";
		else if (_type == LADSPA) return "ingen:LADSPAPlugin";
		else if (_type == DSSI) return "ingen:DSSIPlugin";
		else if (_type == Internal) return "ingen:InternalPlugin";
		else if (_type == Patch) return "ingen:Patch";
		else return "";
	}
	
	/** DEPRECATED */
	void set_type(const string& type_string) {
		if (type_string == "LV2") _type = LV2;
		else if (type_string == "LADSPA") _type = LADSPA;
		else if (type_string == "DSSI") _type = DSSI;
		else if (type_string == "Internal") _type = Internal;
		else if (type_string == "Patch") _type = Patch;
	}
	
	void set_type_from_uri(const string& type_uri) {
		if (type_uri.substr(0, 6) != "ingen:") {
			cerr << "INVALID TYPE STRING!" << endl;
		} else {
			set_type(type_uri.substr(6));
		}
	}

	string default_node_name() { return Raul::Path::nameify(_name); }

#ifdef HAVE_SLV2
	SLV2Plugin slv2_plugin() { return _slv2_plugin; }
#endif

private:
	Type   _type;
	string _uri;
	string _name;

#ifdef HAVE_SLV2
	SLV2Plugin _slv2_plugin;
#endif
};


} // namespace Client
} // namespace Ingen

#endif // PLUGINMODEL_H

