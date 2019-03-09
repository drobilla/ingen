/*
  This file is part of Ingen.
  Copyright 2018 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ingen/FilePath.hpp"

#include <algorithm>
#include <string>
#include <utility>

namespace ingen {

template <typename Char>
static bool
is_sep(const Char chr)
{
#ifdef USE_WINDOWS_FILE_PATHS
	return chr == L'/' || chr == preferred_separator;
#else
	return chr == '/';
#endif
}

FilePath&
FilePath::operator=(FilePath&& path) noexcept
{
	_str = std::move(path._str);
	path.clear();
	return *this;
}

FilePath&
FilePath::operator=(string_type&& str)
{
	_str = std::move(str);
	return *this;
}

FilePath&
FilePath::operator/=(const FilePath& path)
{
	const FilePath::string_type& str = path.string();
	if (!_str.empty() && !is_sep(_str.back()) && !str.empty() &&
	    !is_sep(str.front())) {
		_str += preferred_separator;
	}

	_str += str;
	return *this;
}

FilePath&
FilePath::operator+=(const FilePath& path)
{
	return operator+=(path.native());
}

FilePath&
FilePath::operator+=(const string_type& str)
{
	_str += str;
	return *this;
}

FilePath&
FilePath::operator+=(const value_type* str)
{
	_str += str;
	return *this;
}

FilePath&
FilePath::operator+=(value_type chr)
{
	_str += chr;
	return *this;
}

FilePath&
FilePath::operator+=(serd::StringView sv)
{
	_str.append(sv.data(), sv.size());
	return *this;
}

FilePath
FilePath::root_name()
{
#ifdef USE_WINDOWS_FILE_PATHS
	if (_str.length() >= 2 && _str[0] >= 'A' && _str[0] <= 'Z' &&
	    _str[1] == ':') {
		return FilePath(_str.substr(0, 2));
	}
#endif

	return FilePath();
}

FilePath
FilePath::root_directory() const
{
#ifdef USE_WINDOWS_FILE_PATHS
	const auto name = root_name().string();
	return name.empty() ? Path() : Path(name + preferred_separator);
#endif

	return _str[0] == '/' ? FilePath("/") : FilePath();
}

FilePath
FilePath::root_path() const
{
#ifdef USE_WINDOWS_FILE_PATHS
	const auto name = root_name();
	return name.empty() ? FilePath() : name / root_directory();
#endif
	return root_directory();
}

FilePath
FilePath::relative_path() const
{
	const auto root = root_path();
	return root.empty() ? FilePath()
		: FilePath(_str.substr(root.string().length()));
}

FilePath
FilePath::parent_path() const
{
	if (empty() || *this == root_path()) {
		return *this;
	}

	const auto first_sep = find_first_sep();
	const auto last_sep  = find_last_sep();
	return ((last_sep == std::string::npos || last_sep == first_sep)
	        ? root_path()
	        : FilePath(_str.substr(0, last_sep)));
}

FilePath
FilePath::filename() const
{
	return ((empty() || *this == root_path())
	        ? FilePath()
	        : FilePath(_str.substr(find_last_sep() + 1)));
}

FilePath
FilePath::stem() const
{
	const auto name = filename();
	const auto dot  = name.string().find('.');
	return ((dot == std::string::npos) ? name
	        : FilePath(name.string().substr(0, dot)));
}

FilePath
FilePath::extension() const
{
	const auto name = filename().string();
	const auto dot  = name.find('.');
	return ((dot == std::string::npos) ? FilePath()
	        : FilePath(name.substr(dot, dot)));
}

bool
FilePath::is_absolute() const
{
#ifdef USE_WINDOWS_FILE_PATHS
	return !root_name().empty();
#else
	return !root_directory().empty();
#endif
}

std::size_t
FilePath::find_first_sep() const
{
	const auto i = std::find_if(_str.begin(), _str.end(), is_sep<value_type>);
	return i == _str.end() ? std::string::npos : (i - _str.begin());
}

std::size_t
FilePath::find_last_sep() const
{
	const auto i = std::find_if(_str.rbegin(), _str.rend(), is_sep<value_type>);
	return (i == _str.rend() ? std::string::npos
	        : (_str.length() - 1 - (i - _str.rbegin())));
}

bool
operator==(const FilePath& lhs, const FilePath& rhs) noexcept
{
	return lhs.string() == rhs.string();
}

bool
operator!=(const FilePath& lhs, const FilePath& rhs) noexcept
{
	return !(lhs == rhs);
}

bool
operator<(const FilePath& lhs, const FilePath& rhs) noexcept
{
	return lhs.string().compare(rhs.string()) < 0;
}

bool
operator<=(const FilePath& lhs, const FilePath& rhs) noexcept
{
	return !(rhs < lhs);
}

bool
operator>(const FilePath& lhs, const FilePath& rhs) noexcept
{
	return rhs < lhs;
}

bool
operator>=(const FilePath& lhs, const FilePath& rhs) noexcept
{
	return !(lhs < rhs);
}

FilePath
operator/(const FilePath& lhs, const FilePath& rhs)
{
	return FilePath(lhs) /= rhs;
}

} // namespace ingen
