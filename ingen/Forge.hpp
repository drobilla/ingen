/*
  This file is part of Ingen.
  Copyright 2007-2013 David Robillard <http://drobilla.net/>

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

#include <string>

#include "ingen/Atom.hpp"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"

namespace Ingen {

class URIMap;

/** Forge for Atoms.
 * @ingroup IngenShared
 */
class Forge : public LV2_Atom_Forge {
public:
	explicit Forge(URIMap& map);

	std::string str(const Atom& atom);

	Atom make()          { return Atom(); }
	Atom make(int32_t v) { return Atom(sizeof(v), Int, &v); }
	Atom make(float v)   { return Atom(sizeof(v), Float, &v); }
	Atom make(bool v) {
		const int32_t iv = v ? 1 : 0;
		return Atom(sizeof(int32_t), Bool, &iv);
	}

	Atom make_urid(int32_t v) { return Atom(sizeof(int32_t), URID, &v); }

	Atom alloc(uint32_t size, uint32_t type, const void* val) {
		return Atom(size, type, val);
	}

	Atom alloc(const char* v) {
		const size_t len = strlen(v);
		return Atom(len + 1, String, v);
	}

	Atom alloc(const std::string& v) {
		return Atom(v.length() + 1, String, v.c_str());
	}

	Atom alloc_uri(const char* v) {
		const size_t len = strlen(v);
		return Atom(len + 1, URI, v);
	}

	Atom alloc_uri(const std::string& v) {
		return Atom(v.length() + 1, URI, v.c_str());
	}

private:
	URIMap& _map;
};

} // namespace Ingen

#endif // INGEN_FORGE_HPP
