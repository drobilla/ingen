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

#ifndef RDFQUERY_H
#define RDFQUERY_H

#include <map>
#include <list>
#include <glibmm/ustring.h>

namespace Ingen {
namespace Client {


/** Pretty wrapper for a SPARQL query.
 *
 * Automatically handles things like prepending prefixes, etc.  Ingen specific.
 */
class RDFQuery {
public:
	typedef std::map<Glib::ustring, Glib::ustring> Bindings;
	typedef std::list<Bindings>                    Results;

	RDFQuery(Glib::ustring query)
	{
		const char* const _prefix_header =
			"PREFIX rdf:   <http://www.w3.org/1999/02/22-rdf-syntax-ns#>\n"
			"PREFIX xsd:   <http://www.w3.org/2001/XMLSchema#>\n"
			"PREFIX ingen: <http://codeson.net/ns/ingen#>\n"
			"PREFIX lv2:   <http://lv2plug.in/ontology#>\n";

		_query = _prefix_header + query;
	}

	Results run(const Glib::ustring base_uri) const;

	Glib::ustring string() const { return _query; };

private:

	Glib::ustring _query;
};


} // namespace Client
} // namespace Ingen

#endif // RDFQUERY_H

