/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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

#ifndef INGEN_INTERFACE_PLUGIN_HPP
#define INGEN_INTERFACE_PLUGIN_HPP

#include <string>
#include "raul/URI.hpp"
#include "interface/Resource.hpp"

namespace Ingen {
namespace Shared {


class Plugin : virtual public Resource
{
public:
	enum Type { NIL, LV2, LADSPA, Internal, Patch };

	virtual Type type() const = 0;

	static inline const Raul::URI& type_uri(Type type) {
		static const Raul::URI uris[] = {
			"http://drobilla.net/ns/ingen#nil",
			"http://lv2plug.in/ns/lv2core#Plugin",
			"http://drobilla.net/ns/ingen#LADSPAPlugin",
			"http://drobilla.net/ns/ingen#Internal",
			"http://drobilla.net/ns/ingen#Patch"
		};

		return uris[type];
	}

	inline const Raul::URI& type_uri() const { return type_uri(type()); }

	static inline Type type_from_uri(const Raul::URI& uri) {
		if (uri == type_uri(LV2))
			return LV2;
		else if (uri == type_uri(LADSPA))
			return LADSPA;
		else if (uri == type_uri(Internal))
			return Internal;
		else if (uri == type_uri(Patch))
			return Patch;
		else
			return NIL;
	}
};


} // namespace Shared
} // namespace Ingen

#endif // INGEN_INTERFACE_PLUGIN_HPP
