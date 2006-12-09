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

#ifndef NAMESPACES_H
#define NAMESPACES_H

#include <map>
#include <string>
using std::string;

namespace Ingen {
namespace Client {


/** Collection of RDF namespaces with prefixes.
 */
class Namespaces {
public:
	void add(string prefix, string uri) { _namespaces[prefix] = uri; }
	
	string qualify(string uri);

private:
	std::map<string, string> _namespaces; ///< (prefix, URI)
};


} // namespace Client
} // namespace Ingen

#endif // NAMESPACES_H
