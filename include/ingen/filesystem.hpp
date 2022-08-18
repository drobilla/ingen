/*
  This file is part of Ingen.
  Copyright 2007-2017 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_FILESYSTEM_HPP
#define INGEN_FILESYSTEM_HPP

#include "ingen/FilePath.hpp"

#ifdef _WIN32
#    include <windows.h>
#    include <io.h>
#    define F_OK 0
#    define mkdir(path, flags) _mkdir(path)
#else
#    include <unistd.h>
#endif

#include <sys/stat.h>

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <memory>
#include <vector>

/* A minimal subset of the std::filesystem API from C++17. */

namespace ingen {
namespace filesystem {

inline bool exists(const FilePath& path)
{
	return !access(path.c_str(), F_OK);
}

inline bool is_directory(const FilePath& path)
{
	struct stat info{};
	stat(path.c_str(), &info);
	return S_ISDIR(info.st_mode);
}

inline bool create_directories(const FilePath& path)
{
	std::vector<FilePath> paths;
	for (FilePath p = path; p != path.root_directory(); p = p.parent_path()) {
		paths.emplace_back(p);
	}

	for (auto p = paths.rbegin(); p != paths.rend(); ++p) {
		if (mkdir(p->c_str(), 0755) && errno != EEXIST) {
			return false;
		}
	}

	return true;
}

inline FilePath current_path()
{
	struct Freer { void operator()(char* const ptr) { free(ptr); } };

	std::unique_ptr<char, Freer> cpath(realpath(".", nullptr));

	return FilePath(cpath.get());
}

} // namespace filesystem
} // namespace ingen

#endif // INGEN_FILESYSTEM_HPP
