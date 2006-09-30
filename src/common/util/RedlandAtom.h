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

#include <redland.h>
#include "util/Atom.h"

#define U(x) ((const unsigned char*)(x))

/** Support for serializing an Atom to/from RDF (via redland aka librdf).
 *
 * (Here to prevent a unnecessary redland dependency for Atom).
 */
class RedlandAtom {
public:
	static librdf_node* atom_to_node(librdf_world* world, const Atom& atom) {
		char tmp_buf[32];

		switch (atom.type()) {
		//case NIL:
			// (see below)
			//break;
		case Atom::INT:
			snprintf(tmp_buf, 32, "%d", atom.get_int32());
			return librdf_new_node_from_typed_literal(world, U(tmp_buf), NULL, librdf_new_uri(world, U("http://www.w3.org/2001/XMLSchema#integer")));
				break;
		case Atom::FLOAT:
			snprintf(tmp_buf, 32, "%f", atom.get_float());
			return librdf_new_node_from_typed_literal(world, U(tmp_buf), NULL, librdf_new_uri(world, U("http://www.w3.org/2001/XMLSchema#float")));
			break;
		case Atom::STRING:
			return librdf_new_node_from_literal(world, U(atom.get_string()), NULL, 0);
		case Atom::BLOB:
			cerr << "WARNING: Unserializable atom!" << endl;
			return NULL;
		default: // This catches Atom::Type::NIL too
			return librdf_new_node(world); // blank node
		}
	}

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

};


#endif // REDLAND_ATOM_H
