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

#ifndef INGEN_INTERFACE_PORTTYPE_HPP
#define INGEN_INTERFACE_PORTTYPE_HPP

#include "raul/URI.hpp"

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

namespace Ingen {

/** The type of a port.
 *
 * This type refers to the type of the port itself (not necessarily the type
 * of its contents).  Ports with different types can contain the same type of
 * data, but may e.g. have different access semantics.
 */
class PortType {
public:
	enum Symbol {
		UNKNOWN = 0,
		AUDIO   = 1,
		CONTROL = 2,
		CV      = 3,
		ATOM    = 4
	};

	PortType(const Raul::URI& uri)
		: _symbol(UNKNOWN)
	{
		if (uri == type_uri(AUDIO)) {
			_symbol = AUDIO;
		} else if (uri == type_uri(CONTROL)) {
			_symbol = CONTROL;
		} else if (uri == type_uri(CV)) {
			_symbol = CV;
		} else if (uri == type_uri(ATOM)) {
			_symbol = ATOM;
		}
	}

	PortType(Symbol symbol)
		: _symbol(symbol)
	{}

	inline const Raul::URI& uri()    const { return type_uri(_symbol); }
	inline Symbol           symbol() const { return _symbol; }

	inline bool operator==(const Symbol& symbol) const { return (_symbol == symbol); }
	inline bool operator!=(const Symbol& symbol) const { return (_symbol != symbol); }
	inline bool operator==(const PortType& type) const { return (_symbol == type._symbol); }
	inline bool operator!=(const PortType& type) const { return (_symbol != type._symbol); }
	inline bool operator<(const PortType& type)  const { return (_symbol < type._symbol); }

	inline bool is_audio()   { return _symbol == AUDIO; }
	inline bool is_control() { return _symbol == CONTROL; }
	inline bool is_cv()      { return _symbol == CV; }
	inline bool is_atom()    { return _symbol == ATOM; }

private:
	static inline const Raul::URI& type_uri(unsigned symbol_num) {
		assert(symbol_num <= ATOM);
		static const Raul::URI uris[] = {
			"http://drobilla.net/ns/ingen#nil",
			LV2_CORE__AudioPort,
			LV2_CORE__ControlPort,
			LV2_CORE__CVPort,
			LV2_ATOM__AtomPort
		};
		return uris[symbol_num];
	}

	Symbol _symbol;
};

} // namespace Ingen

#endif // INGEN_INTERFACE_PORTTYPE_HPP
