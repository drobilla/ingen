/* This file is part of Ingen.
 * Copyright (C) 2008 Dave Robillard <http://drobilla.net>
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

#include <iostream>
#include "runtime_paths.hpp"
#include <glibmm/module.h>

using namespace std;

namespace Ingen {
namespace Shared {

static std::string bundle_path;


/** Must be called once at startup, and passed a pointer to a function
 * that lives in the 'top level' of the bundle (e.g. the executable).
 * Passing a function defined in a module etc. will not work!
 */
void
set_bundle_path_from_code(void* function)
{
	Dl_info dli;
	dladdr(function, &dli);

	char bin_loc[PATH_MAX];
	realpath(dli.dli_fname, bin_loc);
	
#ifdef BUNDLE
	string bundle = bin_loc;
	bundle = bundle.substr(0, bundle.find_last_of("/"));
	bundle_path = bundle;;
#endif
}


/** Return the absolute path of a 'resource' file.
 */
std::string
data_file_path(const std::string& name)
{
	std::string ret;
#ifdef BUNDLE
	ret = bundle_path + "/" + INGEN_DATA_DIR + "/" + name;
#else
	ret = std::string(INGEN_DATA_DIR) + "/" + name;
#endif
	return ret;
}


/** Return the absolute path of a module (dynamically loaded shared library).
 */
std::string
module_path(const std::string& name)
{
	std::string ret;
#ifdef BUNDLE
	ret = Glib::Module::build_path(bundle_path + "/" + INGEN_MODULE_DIR, name);
#else
	ret = Glib::Module::build_path(INGEN_MODULE_DIR, name);
#endif
	return ret;
}


} // namespace Ingen
} // namespace Shared

