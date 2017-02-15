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

#include "raul/URI.hpp"
#include "raul/Path.hpp"

namespace Ingen {

inline Raul::URI main_uri() { return Raul::URI("ingen:/main"); }

inline bool uri_is_path(const Raul::URI& uri)
{
	const size_t root_len = main_uri().length();
	if (uri == main_uri()) {
		return true;
	} else {
		return uri.substr(0, root_len + 1) == main_uri() + "/";
	}
}

inline Raul::Path uri_to_path(const Raul::URI& uri)
{
	return (uri == main_uri())
		? Raul::Path("/")
		: Raul::Path(uri.substr(main_uri().length()));
}

inline Raul::URI path_to_uri(const Raul::Path& path)
{
	return Raul::URI(main_uri() + path.c_str());
}

} // namespace Ingen

#endif // INGEN_PATHS_HPP
