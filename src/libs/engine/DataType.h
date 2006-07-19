/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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

namespace Ingen
{


/** A data type that can be stored in a Port.
 *
 * Eventually the goal is to be able to just have to create a new one of these
 * to support a new data type.
 */
class DataType {
public:

	enum Symbol {
		UNKNOWN = 0,
		FLOAT   = 1,
		MIDI    = 2
	};

	DataType(const string& uri)
	: _symbol(UNKNOWN)
	{
		if (uri== type_uris[MIDI]) {
			_symbol = MIDI;
		} else if (uri == type_uris[FLOAT]) {
			_symbol = FLOAT;
		}
	}

	DataType(Symbol symbol)
	: _symbol(symbol)
	{}

	const char* const uri()    const { return type_uris[_symbol]; }
	const Symbol&     symbol() const { return _symbol; }

	inline bool operator==(const Symbol& symbol) const { return (_symbol == symbol); }
	inline bool operator!=(const Symbol& symbol) const { return (_symbol != symbol); }
	inline bool operator==(const DataType& type) const { return (_symbol == type._symbol); }
	inline bool operator!=(const DataType& type) const { return (_symbol != type._symbol); }
private:
	Symbol _symbol;

	// Defined in Port.cpp for no good reason
	static const char* const type_uris[3];
};



} // namespace Ingen

#endif // DATATYPE_H
