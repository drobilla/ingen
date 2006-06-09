/* This file is part of Om.  Copyright (C) 2005 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef PATH_H
#define PATH_H

#include <string>
#include <cassert>
using std::string;

namespace Om {

	
/** Simple wrapper around standard string with useful path-specific methods.
 */
class Path : public std::basic_string<char> {
public:
	Path(const std::basic_string<char>& path)
	: std::basic_string<char>(path)
	{
		assert(path.length() > 0);
	}
	
	
	inline bool is_valid() const
	{
		return (find_last_of("/") != string::npos);
	}
	
	/** Return the name of this object (everything after the last '/').
	 */
	inline std::basic_string<char> name() const
	{
		assert(is_valid());

		if ((*this) == "/")
			return "";
		else
			return substr(find_last_of("/")+1);
	}
	
	
	/** Return the parent's path.
	 *
	 * Calling this on the path "/" will return "/".
	 */
	inline Path parent() const
	{
		assert(is_valid());
		
		std::basic_string<char> parent = substr(0, find_last_of("/"));
		return (parent == "") ? "/" : parent;
	}
	
	inline Path base_path() const
	{
		assert(is_valid());
		
		if ((*this) == "/")
			return *this;
		else
			return (*this) + "/";
	}

	Path(const char* s) : std::basic_string<char>(s) {}
};
	

} // namespace Om

#endif // PATH_H
