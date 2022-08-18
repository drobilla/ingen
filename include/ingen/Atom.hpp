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

#ifndef INGEN_ATOM_HPP
#define INGEN_ATOM_HPP

#include "ingen/ingen.h"
#include "lv2/atom/atom.h"
#include "lv2/urid/urid.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace ingen {

/**
   A generic typed data container.

   An Atom holds a value with some type and size, both specified by a uint32_t.
   Values with size less than sizeof(void*) are stored inline: no dynamic
   allocation occurs so Atoms may be created in hard real-time threads.
   Otherwise, if the size is larger than sizeof(void*), the value will be
   dynamically allocated in a separate chunk of memory.

   In either case, the data is stored in a binary compatible format to LV2_Atom
   (i.e., if the value is dynamically allocated, the header is repeated there).
*/
class INGEN_API Atom
{
public:
	Atom() noexcept = default;

	~Atom() { dealloc(); }

	/** Construct a raw atom.
	 *
	 * Typically this is not used directly, use Forge methods to make atoms.
	 */
	Atom(uint32_t size, LV2_URID type, const void* body)
	    : _atom{size, type}
	{
		if (is_reference()) {
			_body.ptr = static_cast<LV2_Atom*>(malloc(sizeof(LV2_Atom) + size));
			memcpy(_body.ptr, &_atom, sizeof(LV2_Atom));
		}
		if (body) {
			memcpy(get_body(), body, size);
		}
	}

	Atom(const Atom& copy)
		: _atom{copy._atom}
	{
		if (is_reference()) {
			_body.ptr =
			    static_cast<LV2_Atom*>(malloc(sizeof(LV2_Atom) + _atom.size));

			memcpy(_body.ptr, copy._body.ptr, sizeof(LV2_Atom) + _atom.size);
		} else {
			_body.val = copy._body.val;
		}
	}

	Atom& operator=(const Atom& other) {
		if (&other == this) {
			return *this;
		}
		dealloc();
		_atom = other._atom;
		if (is_reference()) {
			_body.ptr =
			    static_cast<LV2_Atom*>(malloc(sizeof(LV2_Atom) + _atom.size));

			memcpy(_body.ptr, other._body.ptr, sizeof(LV2_Atom) + _atom.size);
		} else {
			_body.val = other._body.val;
		}
		return *this;
	}

	bool operator==(const Atom& other) const {
		if (_atom.type != other._atom.type ||
		    _atom.size != other._atom.size) {
			return false;
		}

		return is_reference()
			? !memcmp(_body.ptr, other._body.ptr, sizeof(LV2_Atom) + _atom.size)
			: _body.val == other._body.val;
	}

	bool operator!=(const Atom& other) const {
		return !operator==(other);
	}

	bool operator<(const Atom& other) const {
		if (_atom.type == other._atom.type) {
			const uint32_t min_size = std::min(_atom.size, other._atom.size);
			const int cmp           = is_reference()
				? memcmp(_body.ptr, other._body.ptr, min_size)
				: memcmp(&_body.val, &other._body.val, min_size);
			return cmp < 0 || (cmp == 0 && _atom.size < other._atom.size);
		}

		return type() < other.type();
	}

	/** Like assignment, but only works for value atoms (not references).
	 * Always real-time safe.
	 * @return true iff set succeeded.
	 */
	bool set_rt(const Atom& other) {
		if (is_reference()) {
			return false;
		}

		_atom     = other._atom;
		_body.val = other._body.val;
		return true;
	}

	uint32_t size()     const { return _atom.size; }
	LV2_URID type()     const { return _atom.type; }
	bool     is_valid() const { return _atom.type; }

	const void* get_body() const {
		return is_reference() ? static_cast<void*>(_body.ptr + 1) : &_body.val;
	}

	void* get_body() {
		return is_reference() ? static_cast<void*>(_body.ptr + 1) : &_body.val;
	}

	template <typename T> const T& get() const {
		assert(size() == sizeof(T));
		return *static_cast<const T*>(get_body());
	}

	template <typename T> const T* ptr() const {
		return static_cast<const T*>(get_body());
	}

	const LV2_Atom* atom() const {
		return is_reference() ? _body.ptr : &_atom;
	}

private:
	/** Free dynamically allocated value, if applicable. */
	void dealloc() {
		if (is_reference()) {
			free(_body.ptr);
		}
	}

	/** Return true iff this value is dynamically allocated. */
	bool is_reference() const {
		return _atom.size > sizeof(_body.val);
	}

	LV2_Atom _atom = {0, 0};
	union
	{
		intptr_t  val;
		LV2_Atom* ptr;
	} _body = {};
};

} // namespace ingen

#endif // INGEN_ATOM_HPP
