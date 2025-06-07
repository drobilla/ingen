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

#ifndef INGEN_RUNTIME_PATHS_HPP
#define INGEN_RUNTIME_PATHS_HPP

#include <ingen/FilePath.hpp>
#include <ingen/ingen.h>

#include <string>
#include <vector>

namespace ingen {

INGEN_API void set_bundle_path(const char* path);
INGEN_API void set_bundle_path_from_code(void (*function)());

INGEN_API FilePath
find_in_search_path(const std::string&           name,
                    const std::vector<FilePath>& search_path);

INGEN_API FilePath bundle_file_path(const std::string& name);
INGEN_API FilePath data_file_path(const std::string& name);
INGEN_API FilePath ingen_module_path(const std::string& name);

INGEN_API FilePath              user_config_dir();
INGEN_API FilePath              user_data_dir();
INGEN_API std::vector<FilePath> system_config_dirs();
INGEN_API std::vector<FilePath> system_data_dirs();
INGEN_API std::vector<FilePath> config_dirs();
INGEN_API std::vector<FilePath> data_dirs();
INGEN_API std::vector<FilePath> ingen_module_dirs();

} // namespace ingen

#endif // INGEN_RUNTIME_PATHS_HPP
