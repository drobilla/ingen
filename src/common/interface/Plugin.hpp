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

#include <string>
#include <iostream>
#include "interface/Resource.hpp"

namespace Ingen {
namespace Shared {


class Plugin : virtual public Resource
{
public:
	enum Type { LV2, LADSPA, Internal, Patch };

	virtual Type type() const = 0;
	
	inline const char* type_uri() const {
		switch (type()) {
		case LV2:      return "lv2:Plugin";
		case LADSPA:   return "ingen:LADSPAPlugin";
		case Internal: return "ingen:Internal";
		case Patch:    return "ingen:Patch";
		default:       return "";
		}
	}
	
	static inline Type type_from_uri(const std::string& uri) {
		if (uri == "lv2:Plugin")
			return LV2;
		else if (uri == "ingen:LADSPAPlugin")
			return LADSPA;
		else if (uri == "ingen:Internal")
			return Internal;
		else if (uri == "ingen:Patch")
			return Patch;
		else
			std::cerr << "WARNING: Unknown plugin type " << uri << std::endl;
		return Internal;
	}
};


} // namespace Shared
} // namespace Ingen

#endif // PLUGIN_H
