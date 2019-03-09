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

#include "ingen/types.hpp"
#include "lv2/atom/atom.h"
#include "lv2/atom/forge.h"
#include "lv2/atom/util.h"
#include "sord/sordmm.hpp"
#include "sratom/sratom.h"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace ingen {

/// An atom forge that writes to an automatically-resized memory buffer
class AtomForge : public LV2_Atom_Forge
{
public:
	explicit AtomForge(LV2_URID_Map& map)
	    : _size{0}
	    , _capacity{8 * sizeof(LV2_Atom)}
	    , _sratom{sratom_new(&map)}
	    , _buf{(LV2_Atom*)calloc(8, sizeof(LV2_Atom))}
	{
		lv2_atom_forge_init(this, &map);
		lv2_atom_forge_set_sink(this, c_append, c_deref, this);
	}

	/// Forge an atom from `node` in `model`
	void read(Sord::World& world, SordModel* model, const SordNode* node)
	{
		sratom_read(_sratom.get(), this, world.c_obj(), model, node);
	}

	/// Return the top-level atom that has been forged
	const LV2_Atom* atom() const { return _buf.get(); }

	/// Clear the atom buffer and reset the forge
	void clear()
	{
		lv2_atom_forge_set_sink(this, c_append, c_deref, this);
		_size = 0;
		*_buf = {0U, 0U};
	}

	/// Return the internal atom serialiser
	Sratom& sratom() { return *_sratom; }

private:
	struct SratomDeleter { void operator()(Sratom* s) { sratom_free(s); } };

	using AtomPtr = UPtr<LV2_Atom, FreeDeleter<LV2_Atom>>;
	using SratomPtr = UPtr<Sratom, SratomDeleter>;

	/// Append some data and return a reference to its start
	intptr_t append(const void* buf, uint32_t len) {
		// Record offset of the start of this write (+1 to avoid null)
		const intptr_t ref = _size + 1;

		// Update size and reallocate if necessary
		if (lv2_atom_pad_size(_size + len) > _capacity) {
			_capacity = lv2_atom_pad_size(_size + len);
			_buf      = AtomPtr{(LV2_Atom*)realloc(_buf.release(), _capacity)};
		}

		// Append new data
		memcpy((uint8_t*)_buf.get() + _size, buf, len);
		_size += len;
		return ref;
	}

	/// Dereference a reference previously returned by append()
	LV2_Atom* deref(intptr_t ref) {
		/* Make some assumptions and do unnecessary math to appease
		   -Wcast-align.  This is questionable at best, though the forge should
		   only dereference references to aligned atoms. */
		assert((ref - 1) % sizeof(LV2_Atom) == 0);
		return (LV2_Atom*)(_buf.get() + (ref - 1) / sizeof(LV2_Atom));

		// Alternatively:
		// return (LV2_Atom*)((uint8_t*)_buf + ref - 1);
	}

	static LV2_Atom_Forge_Ref
	c_append(void* handle, const void* buf, uint32_t len) {
		return ((AtomForge*)handle)->append(buf, len);
	}

	static LV2_Atom*
	c_deref(void* handle, LV2_Atom_Forge_Ref ref) {
		return ((AtomForge*)handle)->deref(ref);
	}

	size_t    _size;     ///< Current atom size
	size_t    _capacity; ///< Allocated size of atom buffer
	SratomPtr _sratom;   ///< Atom serialiser
	AtomPtr   _buf;      ///< Atom buffer
};

}  // namespace ingen

#endif  // INGEN_ATOMFORGE_HPP
