/* This file is part of Om.  Copyright (C) 2005 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
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
using std::string;

namespace LibOmClient {


/** Model for a plugin available for loading.
 *
 * \ingroup libomclient
 */
class PluginModel
{
public:
	enum Type { LV2, LADSPA, DSSI, Internal, Patch };

	// FIXME: remove
	PluginModel() {}

	PluginModel(const string& type_string, const string& uri)
	: m_uri(uri)
	{ set_type(type_string); }

	PluginModel(Type type)
	: m_type(type)
	{}

	PluginModel(Type type, const string& uri, const string& name)
	: m_type(type),
	  m_uri(uri),
	  m_name(name)
	{}
	
	PluginModel(Type type, const string& lib_name, const string& plug_label, const string& name)
	: m_type(type),
	  m_lib_name(lib_name),
	  m_plug_label(plug_label),
	  m_name(name)
	{}

	//PluginModel() {}

	Type          type() const                { return m_type; }
	void          type(Type t)                { m_type = t; }
	const string& uri() const                 { return m_uri; }
	void          uri(const string& s)        { m_uri = s; }
	const string& lib_name() const            { return m_lib_name; }
	void          lib_name(const string& s)   { m_lib_name = s; }
	const string& plug_label() const          { return m_plug_label; }
	void          plug_label(const string& s) { m_plug_label = s; }
	const string& name() const                { return m_name; }
	void          name(const string& s)       { m_name = s; }
	
	const char* const type_string() const {
		if (m_type == LV2) return "LV2";
		else if (m_type == LADSPA) return "LADSPA";
		else if (m_type == DSSI) return "DSSI";
		else if (m_type == Internal) return "Internal";
		else if (m_type == Patch) return "Patch";
		else return "";
	}

	void set_type(const string& type_string) {
		if (type_string == "LV2") m_type = LV2;
		else if (type_string == "LADSPA") m_type = LADSPA;
		else if (type_string == "DSSI") m_type = DSSI;
		else if (type_string == "Internal") m_type = Internal;
		else if (type_string == "Patch") m_type = Patch;
	}

private:
	// Prevent copies
	PluginModel(const PluginModel& copy);
	PluginModel& operator=(const PluginModel& copy);
	
	Type   m_type;
	string m_uri;
	string m_lib_name;
	string m_plug_label;
	string m_name;
};


} // namespace Om

#endif // PLUGINMODEL_H

