/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_PARSER_HPP
#define INGEN_PARSER_HPP

#include <string>
#include <list>

#include <boost/optional.hpp>

#include "ingen/Node.hpp"
#include "ingen/ingen.h"
#include "raul/Path.hpp"
#include "raul/URI.hpp"

namespace Ingen {

class Interface;
class World;

/**
   Read Ingen objects from Turtle files or strings.

   @ingroup Ingen
*/
class INGEN_API Parser {
public:
	explicit Parser() {}

	virtual ~Parser() {}

	typedef Node::Properties Properties;

	virtual bool parse_file(
		World*                        world,
		Interface*                    target,
		const std::string&            path,
		boost::optional<Raul::Path>   parent = boost::optional<Raul::Path>(),
		boost::optional<Raul::Symbol> symbol = boost::optional<Raul::Symbol>(),
		boost::optional<Properties>   data   = boost::optional<Properties>());

	virtual boost::optional<Raul::URI> parse_string(
		World*                        world,
		Interface*                    target,
		const std::string&            str,
		const std::string&            base_uri,
		boost::optional<Raul::Path>   parent = boost::optional<Raul::Path>(),
		boost::optional<Raul::Symbol> symbol = boost::optional<Raul::Symbol>(),
		boost::optional<Properties>   data   = boost::optional<Properties>());
};

} // namespace Ingen

#endif // INGEN_PARSER_HPP
