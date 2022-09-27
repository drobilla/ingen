/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ingen/Forge.hpp"
#include "ingen/URI.hpp"
#include "ingen/URIMap.hpp"
#include "lv2/atom/forge.h"
#include "lv2/urid/urid.h"

#include <sstream>

namespace ingen {

Forge::Forge(URIMap& map)
	: LV2_Atom_Forge()
	, _map(map)
{
	lv2_atom_forge_init(this, &map.urid_map());
}

Atom
Forge::make_urid(const ingen::URI& u)
{
	const LV2_URID urid = _map.map_uri(u.string());
	return {sizeof(int32_t), URID, &urid};
}

std::string
Forge::str(const Atom& atom, bool quoted)
{
	std::ostringstream ss;
	if (atom.type() == Int) {
		ss << atom.get<int32_t>();
	} else if (atom.type() == Float) {
		ss << atom.get<float>();
	} else if (atom.type() == Bool) {
		ss << (atom.get<int32_t>() ? "true" : "false");
	} else if (atom.type() == URI || atom.type() == Path) {
		ss << (quoted ? "<" : "")
		   << atom.ptr<const char>()
		   << (quoted ? ">" : "");
	} else if (atom.type() == URID) {
		ss << (quoted ? "<" : "")
		   << _map.unmap_uri(atom.get<int32_t>())
		   << (quoted ? ">" : "");
	} else if (atom.type() == String) {
		ss << (quoted ? "\"" : "")
		   << atom.ptr<const char>()
		   << (quoted ? "\"" : "");
	}
	return ss.str();
}

} // namespace ingen
