/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#ifndef DATATYPE_H
#define DATATYPE_H

namespace Ingen {
namespace Shared {


/** A data type that can be stored in a Port.
 *
 * This type refers to the type of the entire buffer, mirroring LV2,
 * e.g. :AudioPort and :ControlPort both are really 32-bit floating point,
 * but they are different port types.
 *
 * FIXME: Event/MIDI/OSC kludges.
 */
class DataType {
public:
	
	enum Symbol {
		UNKNOWN = 0,
		AUDIO   = 1,
		CONTROL = 2,
		EVENT   = 3
	};
	
	DataType(const std::string& uri)
		: _symbol(UNKNOWN)
	{
		if (uri == type_uri(AUDIO)) {
			_symbol = AUDIO;
		} else if (uri == type_uri(CONTROL)) {
			_symbol = CONTROL;
		} else if (uri == type_uri(EVENT)
				|| (uri == "ingen:MIDIPort") || uri == "ingen:OSCPort") {
			_symbol = EVENT;
		}
	}

	DataType(Symbol symbol)
		: _symbol(symbol)
	{}

	inline const char* uri() const { return type_uri(_symbol); }

	inline bool operator==(const Symbol& symbol) const { return (_symbol == symbol); }
	inline bool operator!=(const Symbol& symbol) const { return (_symbol != symbol); }
	inline bool operator==(const DataType& type) const { return (_symbol == type._symbol); }
	inline bool operator!=(const DataType& type) const { return (_symbol != type._symbol); }

	inline bool is_audio()   { return _symbol == AUDIO; }
	inline bool is_control() { return _symbol == CONTROL; }
	inline bool is_event()   { return _symbol == EVENT; }

private:
	
	static inline const char* type_uri(unsigned symbol_num)  {
		switch (symbol_num) {
		case 1:  return "ingen:AudioPort";
		case 2:  return "ingen:ControlPort";
		case 3:  return "ingen:EventPort";
		case 4:  return "ingen:EventPort"; // MIDI
		case 5:  return "ingen:EventPort"; // OSC
		default: return "";
		}
	}

	Symbol _symbol;
};


} // namespace Shared
} // namespace Ingen

using Ingen::Shared::DataType;

#endif // DATATYPE_H
