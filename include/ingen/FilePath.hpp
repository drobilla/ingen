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

#ifndef INGEN_FILEPATH_HPP
#define INGEN_FILEPATH_HPP

#include "ingen/ingen.h"

#include <boost/utility/string_view.hpp> // IWYU pragma: export

#include <cstddef>
#include <ostream>
#include <string>
#include <utility>

#ifndef USE_WINDOWS_FILE_PATHS
#	if defined(_WIN32) && !defined(__CYGWIN__)
#		define USE_WINDOWS_FILE_PATHS 1
#	else
#		define USE_WINDOWS_FILE_PATHS 0
#	endif
#endif

namespace ingen {

/** A path to a file.
 *
 * This is a minimal subset of the std::filesystem::path interface in C++17.
 * Support for Windows paths is only partial and there is no support for
 * character encoding conversion at all.
 */
class INGEN_API FilePath
{
public:
#if USE_WINDOWS_FILE_PATHS
	using value_type = wchar_t;
	static constexpr value_type preferred_separator = L'\\';
#else
	using value_type = char;
	static constexpr value_type preferred_separator = '/';
#endif

	using string_type = std::basic_string<value_type>;

	FilePath()                = default;
	FilePath(const FilePath&) = default;
	FilePath(FilePath&&)      = default;

	FilePath(string_type str) : _str(std::move(str)) {}
	FilePath(const value_type* str) : _str(str) {}
	FilePath(const boost::basic_string_view<value_type>& sv)
		: _str(sv.data(), sv.length())
	{}

	~FilePath() = default;

	FilePath& operator=(const FilePath& path) = default;
	FilePath& operator=(FilePath&& path) noexcept;
	FilePath& operator=(string_type&& str);

	FilePath& operator/=(const FilePath& path);

	FilePath& operator+=(const FilePath& path);
	FilePath& operator+=(const string_type& str);
	FilePath& operator+=(const value_type* str);
	FilePath& operator+=(value_type chr);
	FilePath& operator+=(boost::basic_string_view<value_type> sv);

	void clear() noexcept { _str.clear(); }

	const string_type& native() const noexcept { return _str; }
	const string_type& string() const noexcept { return _str; }
	const value_type*  c_str() const noexcept { return _str.c_str(); }

	operator string_type() const { return _str; }

	static FilePath root_name();

	FilePath root_directory() const;
	FilePath root_path() const;
	FilePath relative_path() const;
	FilePath parent_path() const;
	FilePath filename() const;
	FilePath stem() const;
	FilePath extension() const;

	bool empty() const noexcept { return _str.empty(); }

	bool is_absolute() const;
	bool is_relative() const { return !is_absolute(); }

private:
	std::size_t find_first_sep() const;
	std::size_t find_last_sep() const;

	string_type _str;
};

INGEN_API bool operator==(const FilePath& lhs, const FilePath& rhs) noexcept;
INGEN_API bool operator!=(const FilePath& lhs, const FilePath& rhs) noexcept;
INGEN_API bool operator<(const FilePath& lhs, const FilePath& rhs) noexcept;
INGEN_API bool operator<=(const FilePath& lhs, const FilePath& rhs) noexcept;
INGEN_API bool operator>(const FilePath& lhs, const FilePath& rhs) noexcept;
INGEN_API bool operator>=(const FilePath& lhs, const FilePath& rhs) noexcept;

INGEN_API FilePath operator/(const FilePath& lhs, const FilePath& rhs);

template <typename Char, typename Traits>
std::basic_ostream<Char, Traits>&
operator<<(std::basic_ostream<Char, Traits>& os, const FilePath& path)
{
	return os << path.string();
}

} // namespace ingen

#endif // INGEN_FILEPATH_HPP
