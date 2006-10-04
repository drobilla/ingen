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

#ifndef REDLAND_ATOM_H
#define REDLAND_ATOM_H

#include <sstream>
#include <cstring>
#include <raptor.h>
#include "util/Atom.h"

#define U(x) ((const unsigned char*)(x))

/** Support for serializing an Atom to/from RDF (via redland aka librdf).
 *
 * (Here to prevent a unnecessary redland dependency for Atom).
 */
class RedlandAtom {
public:
	/** Set this atom's value to the object (value) portion of an RDF triple.
	 *
	 * Caller is responsible for calling free(triple->object).
	 */
	static void atom_to_triple_object(raptor_statement* triple, const Atom& atom) {
		std::ostringstream os;

		triple->object_literal_language = NULL;

		switch (atom.type()) {
		case Atom::INT:
			os << atom.get_int32();
			triple->object = (unsigned char*)strdup(os.str().c_str());
			triple->object_type = RAPTOR_IDENTIFIER_TYPE_LITERAL;
			triple->object_literal_datatype = raptor_new_uri(
				U("http://www.w3.org/2001/XMLSchema#integer"));
			break;
		case Atom::FLOAT:
			os << atom.get_float();
			triple->object = (unsigned char*)strdup(os.str().c_str());
			triple->object_type = RAPTOR_IDENTIFIER_TYPE_LITERAL;
			triple->object_literal_datatype = raptor_new_uri(
				U("http://www.w3.org/2001/XMLSchema#float"));
			break;
		case Atom::STRING:
			triple->object = strdup(atom.get_string());
			triple->object_type = RAPTOR_IDENTIFIER_TYPE_LITERAL;
			break;
		case Atom::BLOB:
		case Atom::NIL:
		default:
			cerr << "WARNING: Unserializable Atom!" << endl;
			triple->object = NULL;
			triple->object_type = RAPTOR_IDENTIFIER_TYPE_UNKNOWN;
		}
	}

#if 0
	static Atom node_to_atom(librdf_node* node) {
		/*switch (type) {
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
		}*/
		cerr << "FIXME: node_to_atom\n";
		return Atom();
	}
#endif

};


#endif // REDLAND_ATOM_H
