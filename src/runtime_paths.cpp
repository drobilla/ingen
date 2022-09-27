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
#include "ingen/filesystem.hpp"
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

static std::vector<FilePath>
parse_search_path(const char* search_path, std::vector<FilePath> defaults)
{
	if (!search_path) {
		return defaults;
	}

	std::vector<FilePath> paths;
	std::istringstream    ss{search_path};
	std::string           entry;
	while (std::getline(ss, entry, search_path_separator)) {
		paths.emplace_back(entry);
	}
	return paths;
}

/** Must be called once at startup, and passed a pointer to a function
 * that lives in the 'top level' of the bundle (e.g. the executable).
 * Passing a function defined in a module etc. will not work!
 */
void
set_bundle_path_from_code(void (*function)())
{
	Dl_info dli;
	dladdr(reinterpret_cast<void*>(function), &dli);

#if INGEN_BUNDLED
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

FilePath
find_in_search_path(const std::string&           name,
                    const std::vector<FilePath>& search_path)
{
	for (const auto& dir : search_path) {
		FilePath path = dir / name;
		if (filesystem::exists(path)) {
			return path;
		}
	}

	return FilePath{};
}

/** Return the absolute path of a file in an Ingen LV2 bundle
 */
FilePath
bundle_file_path(const std::string& name)
{
	return bundle_path / name;
}

FilePath
data_file_path(const std::string& name)
{
	return find_in_search_path(FilePath("ingen") / name, data_dirs());
}

std::vector<FilePath>
ingen_module_dirs()
{
#if INGEN_BUNDLED
	const FilePath default_dir = FilePath(bundle_path) / INGEN_MODULE_DIR;
#else
	const FilePath default_dir = INGEN_MODULE_DIR;
#endif

	return parse_search_path(getenv("INGEN_MODULE_PATH"), {default_dir});
}

FilePath
ingen_module_path(const std::string& name)
{
	return find_in_search_path(
		std::string(library_prefix) + "ingen_" + name + library_suffix,
		ingen_module_dirs());
}

FilePath
user_config_dir()
{
	if (const char* xdg_config_home = getenv("XDG_CONFIG_HOME")) {
		return {xdg_config_home};
	}

	if (const char* home = getenv("HOME")) {
		return FilePath(home) / ".config";
	}

	return {};
}

FilePath
user_data_dir()
{
	if (const char* xdg_data_home = getenv("XDG_DATA_HOME")) {
		return {xdg_data_home};
	}

	if (const char* home = getenv("HOME")) {
		return FilePath(home) / ".local/share";
	}

	return {};
}

std::vector<FilePath>
system_config_dirs()
{
	return parse_search_path(getenv("XDG_CONFIG_DIRS"), {"/etc/xdg"});
}

std::vector<FilePath>
config_dirs()
{
	std::vector<FilePath> paths    = system_config_dirs();
	const FilePath        user_dir = user_config_dir();
	if (!user_dir.empty()) {
		paths.insert(paths.begin(), user_dir);
	}
	return paths;
}

std::vector<FilePath>
system_data_dirs()
{
	return parse_search_path(getenv("XDG_DATA_DIRS"),
	                         {"/usr/local/share", "/usr/share"});
}

std::vector<FilePath>
data_dirs()
{
	std::vector<FilePath> paths    = system_data_dirs();
	const FilePath        user_dir = user_data_dir();

#if INGEN_BUNDLED
	paths.insert(paths.begin(), bundle_path / INGEN_DATA_DIR);
#endif

	if (!user_dir.empty()) {
		paths.insert(paths.begin(), user_dir);
	}

	return paths;
}

} // namespace ingen
