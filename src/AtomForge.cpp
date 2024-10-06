/*
  This file is part of Ingen.
  Copyright 2007-2024 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ingen/AtomForge.hpp"

#include "lv2/atom/atom.h"
#include "lv2/atom/forge.h"
#include "lv2/atom/util.h"
#include "lv2/urid/urid.h"
#include "sord/sord.h"
#include "sord/sordmm.hpp"

#include <cassert>
#include <cstring>

namespace ingen {

AtomForge::AtomForge(LV2_URID_Map& map)
    : LV2_Atom_Forge{}
    , _sratom{sratom_new(&map)}
    , _buf{static_cast<LV2_Atom*>(calloc(8, sizeof(LV2_Atom)))}
{
	lv2_atom_forge_init(this, &map);
	lv2_atom_forge_set_sink(this, c_append, c_deref, this);
}

void
AtomForge::read(Sord::World&          world,
                SordModel* const      model,
                const SordNode* const node)
{
	sratom_read(_sratom.get(), this, world.c_obj(), model, node);
}

const LV2_Atom*
AtomForge::atom() const
{
	return _buf.get();
}

void
AtomForge::clear()
{
	lv2_atom_forge_set_sink(this, c_append, c_deref, this);
	_size = 0;
	*_buf = {0U, 0U};
}

Sratom&
AtomForge::sratom()
{
	return *_sratom;
}

intptr_t
AtomForge::append(const void* const data, const uint32_t len)
{
	// Record offset of the start of this write (+1 to avoid null)
	const auto ref = static_cast<intptr_t>(_size + 1U);

	// Update size and reallocate if necessary
	if (lv2_atom_pad_size(_size + len) > _capacity) {
		_capacity = lv2_atom_pad_size(_size + len);

		_buf =
		    AtomPtr{static_cast<LV2_Atom*>(realloc(_buf.release(), _capacity)),
		            FreeDeleter<LV2_Atom>{}};
	}

	// Append new data
	memcpy(reinterpret_cast<uint8_t*>(_buf.get()) + _size, data, len);
	_size += len;
	return ref;
}

LV2_Atom*
AtomForge::deref(const intptr_t ref)
{
	/* Make some assumptions and do unnecessary math to appease
	   -Wcast-align.  This is questionable at best, though the forge should
	   only dereference references to aligned atoms. */
	assert((ref - 1) % sizeof(LV2_Atom) == 0);
	return static_cast<LV2_Atom*>(_buf.get() + (ref - 1) / sizeof(LV2_Atom));

	// Alternatively:
	// return (LV2_Atom*)((uint8_t*)_buf + ref - 1);
}

LV2_Atom_Forge_Ref
AtomForge::c_append(void* const       self,
                    const void* const data,
                    const uint32_t    len)
{
	return static_cast<AtomForge*>(self)->append(data, len);
}

LV2_Atom*
AtomForge::c_deref(void* const self, const LV2_Atom_Forge_Ref ref)
{
	return static_cast<AtomForge*>(self)->deref(ref);
}

} // namespace ingen
