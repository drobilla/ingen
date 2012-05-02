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

#include <sstream>

#include "ingen/shared/Forge.hpp"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"

namespace Ingen {

Forge::Forge(Shared::LV2URIMap& map)
{
	Int    = map.map_uri(LV2_ATOM__Int);
	Float  = map.map_uri(LV2_ATOM__Float);
	Bool   = map.map_uri(LV2_ATOM__Bool);
	URI    = map.map_uri(LV2_ATOM__URI);
	URID   = map.map_uri(LV2_ATOM__URID);
	String = map.map_uri(LV2_ATOM__String);
	Dict   = map.map_uri(LV2_ATOM__Object);
}

std::string
Forge::str(const Raul::Atom& atom)
{
	std::ostringstream ss;
	if (atom.type() == Int) {
		ss << atom.get_int32();
	} else if (atom.type() == Float) {
		ss << atom.get_float();
	} else if (atom.type() == Bool) {
		ss << (atom.get_bool() ? "true" : "false");
	} else if (atom.type() == URI) {
		ss << "<" << atom.get_uri() << ">";
	} else if (atom.type() == String) {
		ss << "\"" << atom.get_string() << "\"";
	}
	return ss.str();
}

} // namespace Ingen
