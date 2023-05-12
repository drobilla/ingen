/*
  This file is part of Ingen.
  Copyright 2007-2023 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_FMT_HPP
#define INGEN_FMT_HPP

#include <boost/format.hpp> // IWYU pragma: export

#include <initializer_list>
#include <string>

namespace ingen {
template <typename... Args>
std::string
fmt(const char* fmt, Args&&... args)
{
	boost::format                     f{fmt}; // NOLINT(misc-const-correctness)
	const std::initializer_list<char> l{
	    (static_cast<void>(f % args), char{})...};

	(void)l;
	return boost::str(f);
}

} // namespace ingen

#endif // INGEN_FMT_HPP
