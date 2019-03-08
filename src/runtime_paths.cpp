/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ingen/runtime_paths.hpp"

#include "ingen/FilePath.hpp"
#include "ingen_config.h"

#include <algorithm>
#include <cstdlib>
#include <dlfcn.h>
#include <sstream>
#include <string>

namespace ingen {

static FilePath bundle_path;

#if defined(__APPLE__)
const char               search_path_separator = ':';
static const char* const library_prefix        = "lib";
static const char* const library_suffix        = ".dylib";
#elif defined(_WIN32) && !defined(__CYGWIN__)
const char               search_path_separator = ';';
static const char* const library_prefix        = "";
static const char* const library_suffix        = ".dll";
#else
const char               search_path_separator = ':';
static const char* const library_prefix        = "lib";
static const char* const library_suffix        = ".so";
#endif

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

	bundle_path = FilePath(bin_loc).parent_path();
}

void
set_bundle_path(const char* path)
{
	bundle_path = FilePath(path);
}

/** Return the absolute path of a file in an Ingen LV2 bundle
 */
FilePath
bundle_file_path(const std::string& name)
{
	return bundle_path / name;
}

/** Return the absolute path of a 'resource' file.
 */
FilePath
data_file_path(const std::string& name)
{
#ifdef BUNDLE
	return bundle_path / INGEN_DATA_DIR / name;
#else
	return FilePath(INGEN_DATA_DIR) / name;
#endif
}

/** Return the absolute path of a module (dynamically loaded shared library).
 */
FilePath
ingen_module_path(const std::string& name, FilePath dir)
{
	FilePath ret;
	if (dir.empty()) {
#ifdef BUNDLE
		dir = FilePath(bundle_path) / INGEN_MODULE_DIR;
#else
		dir = FilePath(INGEN_MODULE_DIR);
#endif
	}

	return dir /
	       (std::string(library_prefix) + "ingen_" + name + library_suffix);
}

FilePath
user_config_dir()
{
	const char* const xdg_config_home = getenv("XDG_CONFIG_HOME");
	const char* const home            = getenv("HOME");

	if (xdg_config_home) {
		return FilePath(xdg_config_home);
	} else if (home) {
		return FilePath(home) / ".config";
	}

	return FilePath();
}

std::vector<FilePath>
system_config_dirs()
{
	const char* const xdg_config_dirs = getenv("XDG_CONFIG_DIRS");

	std::vector<FilePath> paths;
	if (xdg_config_dirs) {
		std::istringstream ss(xdg_config_dirs);
		std::string entry;
		while (std::getline(ss, entry, search_path_separator)) {
			paths.emplace_back(entry);
		}
	} else {
		paths.emplace_back("/etc/xdg");
	}

	return paths;
}

} // namespace ingen
