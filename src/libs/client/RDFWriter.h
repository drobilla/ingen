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

#ifndef RDFWRITER_H
#define RDFWRITER_H

#include <stdexcept>
#include <string>
#include <map>
#include <raptor.h>
#include "raul/Atom.h"
using std::string; using std::map;


class RdfId {
public:
	enum Type { ANONYMOUS, RESOURCE };

	RdfId(Type t, const string& s) : _type(t), _string(s) {}

	Type          type() const      { return _type; }
	const string& to_string() const { return _string; }

private:
	Type   _type;
	string _string; ///< URI or blank node ID, depending on _type
};


class RDFWriter {
public:
	RDFWriter();

	void   setup_prefixes();
	string expand_uri(const string& uri);

	void start_to_filename(const string& filename) throw (std::logic_error);
	void start_to_string()                         throw (std::logic_error);
	string finish()                                throw (std::logic_error);
	
	bool serialization_in_progress() { return (_serializer != NULL); }

	void write(const RdfId& subject,
	           const RdfId& predicate,
	           const RdfId& object);   
	
	void write(const RdfId& subject,
	           const RdfId& predicate,
	           const Atom&  object);

private:
	raptor_serializer*               _serializer;
	unsigned char*                   _string_output;
	map<string, string>              _prefixes;
};


#endif // RDFWRITER_H
