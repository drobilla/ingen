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

#include "ingen/Forge.hpp"
#include "ingen/URIMap.hpp"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"

namespace Ingen {

Forge::Forge(URIMap& map)
	: _map(map)
{
	lv2_atom_forge_init(this, &map.urid_map_feature()->urid_map);
}

std::string
Forge::str(const Atom& atom)
{
	std::ostringstream ss;
	if (atom.type() == Int) {
		ss << atom.get<int32_t>();
	} else if (atom.type() == Float) {
		ss << atom.get<float>();
	} else if (atom.type() == Bool) {
		ss << (atom.get<int32_t>() ? "true" : "false");
	} else if (atom.type() == URI) {
		ss << "<" << atom.ptr<const char*>() << ">";
	} else if (atom.type() == URID) {
		ss << "<" << _map.unmap_uri(atom.get<int32_t>()) << ">";
	} else if (atom.type() == String) {
		ss << "\"" << atom.ptr<const char*>() << "\"";
	}
	return ss.str();
}

} // namespace Ingen
