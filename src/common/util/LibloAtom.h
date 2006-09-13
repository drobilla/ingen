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

#ifndef LIBLO_ATOM_H
#define LIBLO_ATOM_H

#include <lo/lo.h>
#include "util/Atom.h"


/** Support for serializing an Atom to/from liblo messages.
 *
 * (Here to prevent a unnecessary liblo dependency on Atom).
 */
class LibloAtom {
public:
	static void lo_message_add_atom(lo_message m, const Atom& atom) {
		switch (atom.type()) {
		//case NIL:
			// (see below)
			//break;
		case Atom::INT:
			lo_message_add_int32(m, atom.get_int32());
			break;
		case Atom::FLOAT:
			lo_message_add_float(m, atom.get_float());
			break;
		case Atom::STRING:
			lo_message_add_string(m, atom.get_string());
			break;
		case Atom::BLOB:
			// FIXME: is this okay?  what does liblo do?
			lo_message_add_blob(m, const_cast<void*>(atom.get_blob()));
			break;
		default: // This catches Atom::Type::NIL too
			lo_message_add_nil(m);
			break;
		}
	}

	static Atom lo_arg_to_atom(char type, lo_arg* arg) {
		switch (type) {
		case 'i':
			return Atom(arg->i);
		case 'f':
			return Atom(arg->f);
		case 's':
			return Atom(&arg->s);
		//case 'b'
			// FIXME: How to get a blob from a lo_arg?
			//return Atom(arg->b);
		default:
			return Atom();
		}
	}

};


#endif // LIBLO_ATOM_H
