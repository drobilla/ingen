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

#ifndef INGEN_INTERFACE_PORTTYPE_HPP
#define INGEN_INTERFACE_PORTTYPE_HPP

#include "ingen/URI.hpp"

#include "lv2/atom/atom.h"
#include "lv2/core/lv2.h"

#include <cassert>

namespace ingen {

/** The type of a port.
 *
 * This type refers to the type of the port itself (not necessarily the type
 * of its contents).  Ports with different types can contain the same type of
 * data, but may e.g. have different access semantics.
 */
class PortType {
public:
	enum ID {
		UNKNOWN = 0,
		AUDIO   = 1,
		CONTROL = 2,
		CV      = 3,
		ATOM    = 4
	};

	explicit PortType(const URI& uri)
		: _id(UNKNOWN)
	{
		if (uri == type_uri(AUDIO)) {
			_id = AUDIO;
		} else if (uri == type_uri(CONTROL)) {
			_id = CONTROL;
		} else if (uri == type_uri(CV)) {
			_id = CV;
		} else if (uri == type_uri(ATOM)) {
			_id = ATOM;
		}
	}

	PortType(ID id) noexcept : _id(id) {}

	inline const URI& uri() const { return type_uri(_id); }
	inline ID         id() const { return _id; }

	inline bool operator==(const ID& id) const { return (_id == id); }
	inline bool operator!=(const ID& id) const { return (_id != id); }
	inline bool operator==(const PortType& type) const { return (_id == type._id); }
	inline bool operator!=(const PortType& type) const { return (_id != type._id); }
	inline bool operator<(const PortType& type)  const { return (_id < type._id); }

	inline bool is_audio()   { return _id == AUDIO; }
	inline bool is_control() { return _id == CONTROL; }
	inline bool is_cv()      { return _id == CV; }
	inline bool is_atom()    { return _id == ATOM; }

private:
	static inline const URI& type_uri(unsigned id_num) {
		assert(id_num <= ATOM);
		static const URI uris[] = {
			URI("http://www.w3.org/2002/07/owl#Nothing"),
			URI(LV2_CORE__AudioPort),
			URI(LV2_CORE__ControlPort),
			URI(LV2_CORE__CVPort),
			URI(LV2_ATOM__AtomPort)
		};
		return uris[id_num];
	}

	ID _id;
};

} // namespace ingen

#endif // INGEN_INTERFACE_PORTTYPE_HPP
