/*
  This file is part of Ingen.
  Copyright 2007-2017 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_FORGE_HPP
#define INGEN_FORGE_HPP

#include "ingen/Atom.hpp"
#include "ingen/ingen.h"
#include "lv2/atom/forge.h"

#include <cstdint>
#include <cstring>
#include <string>

namespace ingen {

class URIMap;
class URI;

/** Forge for Atoms.
 * @ingroup IngenShared
 */
class INGEN_API Forge : public LV2_Atom_Forge
{
public:
	explicit Forge(URIMap& map);

	std::string str(const Atom& atom, bool quoted);

	bool is_uri(const Atom& atom) const {
		return atom.type() == URI || atom.type() == URID;
	}

	static Atom make()   { return {}; }
	Atom make(int32_t v) { return {sizeof(v), Int, &v}; }
	Atom make(float v)   { return {sizeof(v), Float, &v}; }
	Atom make(bool v) {
		const int32_t iv = v ? 1 : 0;
		return {sizeof(int32_t), Bool, &iv};
	}

	Atom make_urid(int32_t v) { return {sizeof(int32_t), URID, &v}; }

	Atom make_urid(const ingen::URI& u);

	static Atom alloc(uint32_t s, uint32_t t, const void* v) {
		return {s, t, v};
	}

	Atom alloc(const char* v) {
		const auto len = static_cast<uint32_t>(strlen(v));
		return {len + 1U, String, v};
	}

	Atom alloc(const std::string& v) {
		return {static_cast<uint32_t>(v.length()) + 1U, String, v.c_str()};
	}

	Atom alloc_uri(const char* v) {
		const auto len = static_cast<uint32_t>(strlen(v));
		return {len + 1U, URI, v};
	}

	Atom alloc_uri(const std::string& v) {
		return {static_cast<uint32_t>(v.length()) + 1U, URI, v.c_str()};
	}

private:
	URIMap& _map;
};

} // namespace ingen

#endif // INGEN_FORGE_HPP
