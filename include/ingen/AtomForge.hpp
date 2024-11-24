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

#ifndef INGEN_ATOMFORGE_HPP
#define INGEN_ATOMFORGE_HPP

#include <ingen/ingen.h>
#include <ingen/memory.hpp>
#include <lv2/atom/atom.h>
#include <lv2/atom/forge.h>
#include <lv2/urid/urid.h>
#include <sord/sord.h>
#include <sratom/sratom.h>

#include <cstdint>
#include <cstdlib>
#include <memory>

namespace Sord {
class World;
} // namespace Sord

namespace ingen {

/// An atom forge that writes to an automatically-resized memory buffer
class INGEN_API AtomForge : public LV2_Atom_Forge
{
public:
	explicit AtomForge(LV2_URID_Map& map);

	/// Forge an atom from `node` in `model`
	void read(Sord::World& world, SordModel* model, const SordNode* node);

	/// Return the top-level atom that has been forged
	const LV2_Atom* atom() const;

	/// Clear the atom buffer and reset the forge
	void clear();

	/// Return the internal atom serialiser
	Sratom& sratom();

private:
	struct SratomDeleter { void operator()(Sratom* s) { sratom_free(s); } };

	using AtomPtr   = std::unique_ptr<LV2_Atom, FreeDeleter<LV2_Atom>>;
	using SratomPtr = std::unique_ptr<Sratom, SratomDeleter>;

	/// Append some data and return a reference to its start
	intptr_t append(const void* data, uint32_t len);

	/// Dereference a reference previously returned by append()
	LV2_Atom* deref(intptr_t ref);

	static LV2_Atom_Forge_Ref
	c_append(void* self, const void* data, uint32_t len);

	static LV2_Atom*
	c_deref(void* self, LV2_Atom_Forge_Ref ref);

	size_t    _size{0};                        ///< Current atom size
	size_t    _capacity{8 * sizeof(LV2_Atom)}; ///< Allocated size of buffer
	SratomPtr _sratom;                         ///< Atom serialiser
	AtomPtr   _buf;                            ///< Atom buffer
};

} // namespace ingen

#endif // INGEN_ATOMFORGE_HPP
