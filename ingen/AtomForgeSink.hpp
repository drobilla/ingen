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

#ifndef INGEN_ATOMFORGESINK_HPP
#define INGEN_ATOMFORGESINK_HPP

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"

namespace Ingen {

/** A resizing sink for LV2_Atom_Forge. */
class AtomForgeSink
{
public:
	AtomForgeSink(LV2_Atom_Forge* forge = nullptr)
		: _capacity(8 * sizeof(LV2_Atom))
		, _size(0)
		, _buf((LV2_Atom*)calloc(8, sizeof(LV2_Atom)))
	{
		if (forge) {
			set_forge_sink(forge);
		}
	}

	~AtomForgeSink() { free(_buf); }

	void set_forge_sink(LV2_Atom_Forge* forge) {
		lv2_atom_forge_set_sink(forge, c_append, c_deref, this);
	}

	/** Append some data and return a reference to its start. */
	intptr_t append(const void* buf, uint32_t len) {
		// Record offset of the start of this write (+1 to avoid NULL)
		const intptr_t ref = _size + 1;

		// Update size and reallocate if necessary
		if (lv2_atom_pad_size(_size + len) > _capacity) {
			_capacity = lv2_atom_pad_size(_size + len);
			_buf       = (LV2_Atom*)realloc(_buf, _capacity);
		}

		// Append new data
		memcpy((uint8_t*)_buf + _size, buf, len);
		_size += len;
		return ref;
	}

	/** Dereference a reference previously returned by append. */
	LV2_Atom* deref(intptr_t ref) {
		/* Make some assumptions and do unnecessary math to appease
		   -Wcast-align.  This is questionable at best, though the forge should
		   only dereference references to aligned atoms. */
		assert((ref - 1) % sizeof(LV2_Atom) == 0);
		return (LV2_Atom*)(_buf + (ref - 1) / sizeof(LV2_Atom));

		// Alternatively:
		// return (LV2_Atom*)((uint8_t*)_buf + ref - 1);
	}

	const LV2_Atom* atom() const { return _buf; }

	void clear() { _buf->type = 0; _buf->size = 0; _size = 0; }

	static LV2_Atom_Forge_Ref
	c_append(void* handle, const void* buf, uint32_t len) {
		return ((AtomForgeSink*)handle)->append(buf, len);
	}

	static LV2_Atom*
	c_deref(void* handle, LV2_Atom_Forge_Ref ref) {
		return ((AtomForgeSink*)handle)->deref(ref);
	}

private:
	size_t    _capacity;
	size_t    _size;
	LV2_Atom* _buf;
};

}  // namespace Ingen

#endif  // INGEN_ATOMFORGESINK_HPP
