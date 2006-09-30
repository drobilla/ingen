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

#ifndef ATOM_H
#define ATOM_H

#include <cstdlib>
#include <cassert>
#include <cstring>
#include <string>

using std::string;


/** An OSC atom (fundamental data types OSC messages are composed of).
 */
class Atom {
public:
	enum Type {
		NIL,
		INT,
		FLOAT,
		STRING,
		BLOB
	};

	Atom()                  : _type(NIL),    _blob_val(0)                     {}
	Atom(int32_t val)       : _type(INT),    _int_val(val)                    {}
	Atom(float val)         : _type(FLOAT),  _float_val(val)                  {}
	Atom(const char* val)   : _type(STRING), _string_val(strdup(val))         {}
	Atom(const string& val) : _type(STRING), _string_val(strdup(val.c_str())) {}

	Atom(void* val) : _type(BLOB), _blob_size(sizeof(val)), _blob_val(malloc(_blob_size))
	{ memcpy(_blob_val, val, sizeof(_blob_size)); }

	~Atom()
	{
		if (_type == STRING)
			free(_string_val);
		else if (_type == BLOB)
			free(_blob_val);
	}

	// Gotta love C++ boilerplate:
	
	Atom(const Atom& copy)
	: _type(copy._type)
	, _blob_size(copy._blob_size)
	{
		switch (_type) {
		case NIL:    _blob_val   = 0;                        break;
		case INT:    _int_val    = copy._int_val;            break;
		case FLOAT:  _float_val  = copy._float_val;          break;
		case STRING: _string_val = strdup(copy._string_val); break;

		case BLOB:   _blob_val = malloc(_blob_size);
		             memcpy(_blob_val, copy._blob_val, _blob_size);
					 break;

		default: break;
		}
	}
	
	Atom& operator=(const Atom& other)
	{
		if (_type == BLOB)
			free(_blob_val);
		else if (_type == STRING)
			free(_string_val);

		_type = other._type;
		_blob_size = other._blob_size;

		switch (_type) {
		case NIL:    _blob_val   = 0;                         break;
		case INT:    _int_val    = other._int_val;            break;
		case FLOAT:  _float_val  = other._float_val;          break;
		case STRING: _string_val = strdup(other._string_val); break;

		case BLOB:   _blob_val = malloc(_blob_size);
		             memcpy(_blob_val, other._blob_val, _blob_size);
					 break;

		default: break;
		}
		return *this;
	}

	/** Type of this atom.  Always check this before attempting to get the
	 * value - attempting to get the incorrectly typed value is a fatal error.
	 */
	Type type() const { return _type; }

	inline int32_t     get_int32()  const { assert(_type == INT);    return _int_val; }
	inline float       get_float()  const { assert(_type == FLOAT);  return _float_val; }
	inline const char* get_string() const { assert(_type == STRING); return _string_val; }
	inline const void* get_blob()   const { assert(_type == BLOB);   return _blob_val; }

	inline operator bool() const { return (_type != NIL); }

private:
	Type _type;
	
	size_t _blob_size; ///< always a multiple of 32
	
	union {
		int32_t _int_val;
		float   _float_val;
		char*   _string_val;
		void*   _blob_val;
	};
};

#endif // ATOM_H
