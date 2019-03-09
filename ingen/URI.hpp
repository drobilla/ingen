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

#ifndef INGEN_URI_HPP
#define INGEN_URI_HPP

#include "ingen/FilePath.hpp"
#include "ingen/ingen.h"
#include "serd/serd.hpp"

#include <boost/utility/string_view.hpp>

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string>

namespace ingen {

class INGEN_API URI : public serd::Node
{
public:
	using Slice = serd::StringView;

	explicit URI(const std::string& str);
	explicit URI(const char* str);
	URI(const serd::NodeView& node);
	URI(const serd::Node& node);
	explicit URI(const FilePath& path);

	URI make_relative(const URI& base) const;

	std::string string() const { return std::string(*this); }
	std::string str() const { return std::string(*this); }

	FilePath file_path() const {
		return scheme() == "file" ? FilePath(path()) : FilePath();
	}

	Slice scheme()    const { return serd::URI{*this}.scheme(); }
	Slice authority() const { return serd::URI{*this}.authority(); }
	Slice path()      const { return serd::URI{*this}.path(); }
	Slice query()     const { return serd::URI{*this}.query(); }
	Slice fragment()  const { return serd::URI{*this}.fragment(); }

	static bool is_valid(const char* str) {
		return serd_uri_string_has_scheme(str);
	}

	static bool is_valid(const std::string& str)
	{
		return is_valid(str.c_str());
	}
};

inline bool operator==(const URI& lhs, const std::string& rhs)
{
	return lhs.c_str() == rhs;
}

} // namespace ingen

#endif  // INGEN_URI_HPP
