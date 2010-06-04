/* This file is part of Ingen.
 * Copyright (C) 2008-2009 Dave Robillard <http://drobilla.net>
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

#include <dlfcn.h>
#include <limits.h>
#include <stdlib.h>
#include <glibmm/module.h>
#include <glibmm/miscutils.h>
#include "ingen-config.h"
#include "runtime_paths.hpp"

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

#ifdef BUNDLE
	char bin_loc[PATH_MAX];
	realpath(dli.dli_fname, bin_loc);
#else
	const char* bin_loc = dli.dli_fname;
#endif

	string bundle = bin_loc;
	bundle = bundle.substr(0, bundle.find_last_of(G_DIR_SEPARATOR));
	bundle_path = bundle;
}


void
set_bundle_path(const char* path)
{
	bundle_path = path;
}


/** Return the absolute path of a file in an Ingen LV2 bundle
 */
std::string
bundle_file_path(const std::string& name)
{
	return Glib::build_filename(bundle_path, name);
}

/** Return the absolute path of a 'resource' file.
 */
std::string
data_file_path(const std::string& name)
{
#ifdef BUNDLE
	return Glib::build_filename(bundle_path, Glib::build_path(INGEN_DATA_DIR, name));
#else
	return Glib::build_filename(INGEN_DATA_DIR, name);
#endif
}


/** Return the absolute path of a module (dynamically loaded shared library).
 */
std::string
module_path(const std::string& name)
{
	std::string ret;
#ifdef BUNDLE
	ret =  Glib::Module::build_path(Glib::build_path(bundle_path, INGEN_MODULE_DIR), name);
#else
	ret = Glib::Module::build_path(INGEN_MODULE_DIR, name);
#endif
#ifdef __APPLE__
	// MacPorts glib doesnt seem to do portable path building correctly...
	if (ret.substr(ret.length() - 3) == ".so")
		ret = ret.substr(0, ret.length() - 2).append("dylib");
#endif
	return ret;
}


} // namespace Ingen
} // namespace Shared

