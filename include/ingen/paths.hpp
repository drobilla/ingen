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

#ifndef INGEN_PATHS_HPP
#define INGEN_PATHS_HPP

#include "ingen/URI.hpp"
#include "raul/Path.hpp"

#include <cstddef>
#include <string>

namespace ingen {

inline URI main_uri() { return URI("ingen:/main"); }

inline bool uri_is_path(const URI& uri)
{
	const size_t root_len = main_uri().string().length();
	if (uri == main_uri()) {
		return true;
	} else {
		return uri.string().substr(0, root_len + 1) ==
		       main_uri().string() + "/";
	}
}

inline Raul::Path uri_to_path(const URI& uri)
{
	return (uri == main_uri())
		? Raul::Path("/")
		: Raul::Path(uri.string().substr(main_uri().string().length()));
}

inline URI path_to_uri(const Raul::Path& path)
{
	return URI(main_uri().string() + path.c_str());
}

} // namespace ingen

#endif // INGEN_PATHS_HPP
