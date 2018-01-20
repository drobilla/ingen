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

#include <iosfwd>
#include <string>

#include <boost/utility/string_view.hpp>

#include "ingen/ingen.h"
#include "serd/serd.h"
#include "sord/sordmm.hpp"

namespace Ingen {

class INGEN_API URI
{
public:
	using Chunk = boost::string_view;

	URI();
	explicit URI(const std::string& str);
	explicit URI(const char* str);
	URI(const std::string& str, const URI& base);
	explicit URI(const Sord::Node& node);

	URI(const URI& uri);
	URI& operator=(const URI& uri);

	URI(URI&& uri);
	URI& operator=(URI&& uri);

	~URI();

	bool empty() const { return !_node.buf; }

	std::string string() const { return std::string(c_str(), _node.n_bytes); }
	size_t      length() const { return _node.n_bytes; }
	const char* c_str()  const { return (const char*)_node.buf; }

	operator std::string() const { return string(); }

	const char* begin() const { return (const char*)_node.buf; }
	const char* end()   const { return (const char*)_node.buf + _node.n_bytes; }

	Chunk scheme()    const { return make_chunk(_uri.scheme); }
	Chunk authority() const { return make_chunk(_uri.authority); }
	Chunk path()      const { return make_chunk(_uri.path); }
	Chunk query()     const { return make_chunk(_uri.query); }
	Chunk fragment()  const { return make_chunk(_uri.fragment); }

	static bool is_valid(const char* str) {
		return serd_uri_string_has_scheme((const uint8_t*)str);
	}

	static bool is_valid(const std::string& str)
	{
		return is_valid(str.c_str());
	}

private:
	static Chunk make_chunk(const SerdChunk& chunk) {
		return Chunk((const char*)chunk.buf, chunk.len);
	}

	SerdNode _node;
	SerdURI  _uri;
};

inline bool operator==(const URI& lhs, const URI& rhs)
{
	return lhs.string() == rhs.string();
}

inline bool operator==(const URI& lhs, const std::string& rhs)
{
	return lhs.string() == rhs;
}

inline bool operator==(const URI& lhs, const char* rhs)
{
	return lhs.string() == rhs;
}

inline bool operator==(const URI& lhs, const Sord::Node& rhs)
{
	return rhs.type() == Sord::Node::URI && lhs.string() == rhs.to_string();
}

inline bool operator==(const Sord::Node& lhs, const URI& rhs)
{
	return rhs == lhs;
}

inline bool operator!=(const URI& lhs, const URI& rhs)
{
	return lhs.string() != rhs.string();
}

inline bool operator!=(const URI& lhs, const std::string& rhs)
{
	return lhs.string() != rhs;
}

inline bool operator!=(const URI& lhs, const char* rhs)
{
	return lhs.string() != rhs;
}

inline bool operator!=(const URI& lhs, const Sord::Node& rhs)
{
	return !(lhs == rhs);
}

inline bool operator!=(const Sord::Node& lhs, const URI& rhs)
{
	return !(lhs == rhs);
}

inline bool operator<(const URI& lhs, const URI& rhs)
{
	return lhs.string() < rhs.string();
}

template <typename Char, typename Traits>
inline std::basic_ostream<Char, Traits>&
operator<<(std::basic_ostream<Char, Traits>& os, const URI& uri)
{
	return os << uri.string();
}

} // namespace Ingen

#endif  // INGEN_URI_HPP
