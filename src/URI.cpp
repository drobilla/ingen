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

#include "ingen/URI.hpp"

#include "ingen/FilePath.hpp"

#include <cassert>

namespace ingen {

URI::URI(const std::string& str)
	: serd::Node{serd::make_uri(str)}
{
}

URI::URI(const char* str)
	: serd::Node{serd::make_uri(str)}
{
}

URI::URI(const serd::NodeView& node)
	: serd::Node{node}
{
	assert(cobj());
	assert(node.type() == serd::NodeType::URI);
}

URI::URI(const serd::Node& node)
	: serd::Node{node}
{
	assert(cobj());
	assert(node.type() == serd::NodeType::URI);
}

URI::URI(const FilePath& path)
	: serd::Node{serd::make_file_uri(path.string())}
{
}

URI
URI::make_relative(const URI& base) const
{
	return URI(serd::make_relative_uri(std::string(*this), base));
}

}  // namespace ingen
