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

#ifndef PLUGINMODEL_H
#define PLUGINMODEL_H

#include <string>
#include <iostream>
#include "raul/Path.h"
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
	: m_uri(uri),
	  m_name(name)
	{
		set_type_from_uri(type_uri);
	}
	
	Type          type() const                { return m_type; }
	void          type(Type t)                { m_type = t; }
	const string& uri() const                 { return m_uri; }
	void          uri(const string& s)        { m_uri = s; }
	const string& name() const                { return m_name; }
	void          name(const string& s)       { m_name = s; }
	
	/*const char* const type_string() const {
		if (m_type == LV2) return "LV2";
		else if (m_type == LADSPA) return "LADSPA";
		else if (m_type == DSSI) return "DSSI";
		else if (m_type == Internal) return "Internal";
		else if (m_type == Patch) return "Patch";
		else return "";
	}*/

	const char* const type_uri() const {
		if (m_type == LV2) return "ingen:LV2Plugin";
		else if (m_type == LADSPA) return "ingen:LADSPAPlugin";
		else if (m_type == DSSI) return "ingen:DSSIPlugin";
		else if (m_type == Internal) return "ingen:InternalPlugin";
		else if (m_type == Patch) return "ingen:Patch";
		else return "";
	}
	
	/** DEPRECATED */
	void set_type(const string& type_string) {
		if (type_string == "LV2") m_type = LV2;
		else if (type_string == "LADSPA") m_type = LADSPA;
		else if (type_string == "DSSI") m_type = DSSI;
		else if (type_string == "Internal") m_type = Internal;
		else if (type_string == "Patch") m_type = Patch;
	}
	
	void set_type_from_uri(const string& type_uri) {
		if (type_uri.substr(0, 6) != "ingen:") {
			cerr << "INVALID TYPE STRING!" << endl;
		} else {
			set_type(type_uri.substr(6));
		}
	}

	string default_node_name() { return Path::nameify(m_name); }

private:
	Type   m_type;
	string m_uri;
	string m_name;
};


} // namespace Client
} // namespace Ingen

#endif // PLUGINMODEL_H

