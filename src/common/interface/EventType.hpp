/* This file is part of Ingen.
 * Copyright (C) 2008-2009 Dave Robillard <http://drobilla.net>
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

#ifndef INGEN_INTERFACE_EVENTTYPE_HPP
#define INGEN_INTERFACE_EVENTTYPE_HPP

#include "raul/URI.hpp"

namespace Ingen {
namespace Shared {


/** A type of event (that can live in an EventBuffer).
 */
class EventType {
public:

	enum Symbol {
		UNKNOWN = 0,
		MIDI    = 1,
		OSC     = 2
	};

	EventType(const Raul::URI& uri)
		: _symbol(UNKNOWN)
	{
		if (uri == type_uri(MIDI)) {
			_symbol = MIDI;
		} else if (uri == type_uri(OSC)) {
			_symbol = OSC;
		}
	}

	EventType(Symbol symbol)
		: _symbol(symbol)
	{}

	inline const Raul::URI& uri() const { return type_uri(_symbol); }

	inline bool operator==(const Symbol& symbol) const { return (_symbol == symbol); }
	inline bool operator!=(const Symbol& symbol) const { return (_symbol != symbol); }
	inline bool operator==(const EventType& type) const { return (_symbol == type._symbol); }
	inline bool operator!=(const EventType& type) const { return (_symbol != type._symbol); }

	inline bool is_midi() { return _symbol == MIDI; }
	inline bool is_osc()  { return _symbol == OSC; }

private:
	static inline const Raul::URI& type_uri(unsigned symbol_num) {
		assert(symbol_num <= OSC);
		static const Raul::URI uris[] = {
			"http://drobilla.net/ns/ingen#nil",
			"http://drobilla.net/ns/ingen#MidiEvent",
			"http://drobilla.net/ns/ingen#OSCEvent"
		};
		return uris[symbol_num];
	}

	Symbol _symbol;
};


} // namespace Shared
} // namespace Ingen

#endif // INGEN_INTERFACE_EVENTTYPE_HPP
