/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
 *
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef INGEN_INTERFACE_PORTTYPE_HPP
#define INGEN_INTERFACE_PORTTYPE_HPP

#include <raul/URI.hpp>

namespace Ingen {
namespace Shared {


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
		EVENTS  = 3,
		VALUE   = 7,
		MESSAGE = 8,
	};

	PortType(const Raul::URI& uri)
		: _symbol(UNKNOWN)
	{
		if (uri.str() == type_uri(AUDIO)) {
			_symbol = AUDIO;
		} else if (uri.str() == type_uri(CONTROL)) {
			_symbol = CONTROL;
		} else if (uri.str() == type_uri(EVENTS)) {
			_symbol = EVENTS;
		} else if (uri.str() == type_uri(VALUE)) {
			_symbol = VALUE;
		} else if (uri.str() == type_uri(MESSAGE)) {
			_symbol = MESSAGE;
		}
	}

	PortType(Symbol symbol)
		: _symbol(symbol)
	{}

	inline const char* uri()    const { return type_uri(_symbol); }
	inline Symbol      symbol() const { return _symbol; }

	inline bool operator==(const Symbol& symbol) const { return (_symbol == symbol); }
	inline bool operator!=(const Symbol& symbol) const { return (_symbol != symbol); }
	inline bool operator==(const PortType& type) const { return (_symbol == type._symbol); }
	inline bool operator!=(const PortType& type) const { return (_symbol != type._symbol); }

	inline bool is_audio()   { return _symbol == AUDIO; }
	inline bool is_control() { return _symbol == CONTROL; }
	inline bool is_events()  { return _symbol == EVENTS; }
	inline bool is_value()   { return _symbol == VALUE; }
	inline bool is_message() { return _symbol == MESSAGE; }

private:

	static inline const char* type_uri(unsigned symbol_num)  {
		switch (symbol_num) {
		case 1:  return "lv2:AudioPort";
		case 2:  return "lv2:ControlPort";
		case 3:  return "lv2ev:EventPort";
		case 7:  return "obj:ValuePort";
		case 8:  return "obj:MessagePort";
		default: return "";
		}
	}

	Symbol _symbol;
};


} // namespace Shared
} // namespace Ingen

#endif // INGEN_INTERFACE_PORTTYPE_HPP
