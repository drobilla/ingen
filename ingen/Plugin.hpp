/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_PLUGIN_HPP
#define INGEN_PLUGIN_HPP

#include "raul/URI.hpp"

#include "ingen/Resource.hpp"

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

namespace Ingen {

/** A plugin which instantiates to a Node.
 * @ingroup Ingen
 */
class Plugin : virtual public Resource
{
public:
	enum Type { NIL, LV2, Internal, Patch };

	virtual Type type() const = 0;

	static inline const Raul::URI& type_uri(Type type) {
		static const Raul::URI uris[] = {
			"http://drobilla.net/ns/ingen#nil",
			LV2_CORE__Plugin,
			"http://drobilla.net/ns/ingen#Internal",
			"http://drobilla.net/ns/ingen#Patch"
		};

		return uris[type];
	}

	inline const Raul::URI& type_uri() const { return type_uri(type()); }

	static inline Type type_from_uri(const Raul::URI& uri) {
		if (uri == type_uri(LV2))
			return LV2;
		else if (uri == type_uri(Internal))
			return Internal;
		else if (uri == type_uri(Patch))
			return Patch;
		else
			return NIL;
	}
};

} // namespace Ingen

#endif // INGEN_PLUGIN_HPP
