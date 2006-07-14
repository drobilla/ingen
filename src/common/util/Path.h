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
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef PATH_H
#define PATH_H

#include <string>
#include <cassert>
using std::string;

namespace Om {

	
/** Simple wrapper around standard string with useful path-specific methods.
 *
 * This enforces that a Path is a valid OSC path (though it is used for
 * GraphObject paths, which aren't directly OSC paths but a portion of one).
 *
 * A path is divided by slashes (/).  The first character MUST be a slash, and
 * the last character MUST NOT be a slash (except in the special case of the 
 * root path "/", which is the only valid single-character path).
 *
 * Valid characters are the 95 printable ASCII characters (32-126), excluding:
 * space # * , ? [ ] { }
 */
class Path : public std::basic_string<char> {
public:

	/** Construct a Path from an std::string.
	 *
	 * It is a fatal error to construct a Path from an invalid string,
	 * use @ref is_valid first to check.
	 */
	Path(const std::basic_string<char>& path)
	: std::basic_string<char>(path)
	{
		assert(is_valid(path));
	}
	
	/** Construct a Path from a C string.
	 *
	 * It is a fatal error to construct a Path from an invalid string,
	 * use @ref is_valid first to check.
	 */
	Path(const char* cpath)
	: std::basic_string<char>(cpath)
	{
		assert(is_valid(cpath));
	}
	
	static bool is_valid(const std::basic_string<char>& path)
	{
		if (path.length() == 0)
			return false;

		// Must start with a /
		if (path.at(0) != '/')
			return false;
		
		// Must not end with a slash unless "/"
		if (path.length() > 1 && path.at(path.length()-1) == '/')
			return false;
	
		assert(path.find_last_of("/") != string::npos);
		
		// All characters must be printable ASCII
		for (size_t i=0; i < path.length(); ++i)
			if (path.at(i) < 32 || path.at(i) > 126)
				return false;

		// Disallowed characters
		if (       path.find(" ") != string::npos 
				|| path.find("#") != string::npos
				|| path.find("*") != string::npos
				|| path.find(",") != string::npos
				|| path.find("?") != string::npos
				|| path.find("[") != string::npos
				|| path.find("]") != string::npos
				|| path.find("{") != string::npos
				|| path.find("}") != string::npos)
			return false;

		return true;
	}
	
	/** Return the name of this object (everything after the last '/').
	 * This is the "method name" for OSC paths.
	 */
	inline std::basic_string<char> name() const
	{
		if ((*this) == "/")
			return "";
		else
			return substr(find_last_of("/")+1);
	}
	
	
	/** Return the parent's path.
	 *
	 * Calling this on the path "/" will return "/".
	 * This is the (deepest) "container path" for OSC paths.
	 */
	inline Path parent() const
	{
		std::basic_string<char> parent = substr(0, find_last_of("/"));
		return (parent == "") ? "/" : parent;
	}
	
	/** Parent path with a "/" appended.
	 *
	 * Because of the "/" special case, append a child name to base_path()
	 * to construct a path.
	 */
	inline Path base_path() const
	{
		if ((*this) == "/")
			return *this;
		else
			return (*this) + "/";
	}
};
	

} // namespace Om

#endif // PATH_H
