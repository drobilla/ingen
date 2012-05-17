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

/**
   @defgroup IngenSerialisation Turtle Serialisation
*/

#ifndef INGEN_SERIALISATION_PARSER_HPP
#define INGEN_SERIALISATION_PARSER_HPP

#include <string>
#include <list>

#include <boost/optional.hpp>
#include <glibmm/ustring.h>

#include "ingen/GraphObject.hpp"
#include "raul/Path.hpp"

namespace Ingen {

class Interface;

namespace Shared { class World; }

namespace Serialisation {

/**
   Read Ingen objects from Turtle files or strings.

   @ingroup IngenSerialisation
*/
class Parser {
public:
	explicit Parser() {}

	virtual ~Parser() {}

	typedef GraphObject::Properties Properties;

	virtual bool parse_file(
		Shared::World*                world,
		Interface*                    target,
		Glib::ustring                 path,
		boost::optional<Raul::Path>   parent = boost::optional<Raul::Path>(),
		boost::optional<Raul::Symbol> symbol = boost::optional<Raul::Symbol>(),
		boost::optional<Properties>   data   = boost::optional<Properties>());

	virtual bool parse_string(
		Shared::World*                world,
		Interface*                    target,
		const Glib::ustring&          str,
		const Glib::ustring&          base_uri,
		boost::optional<Raul::Path>   parent = boost::optional<Raul::Path>(),
		boost::optional<Raul::Symbol> symbol = boost::optional<Raul::Symbol>(),
		boost::optional<Properties>   data   = boost::optional<Properties>());
};

} // namespace Serialisation
} // namespace Ingen

#endif // INGEN_SERIALISATION_PARSER_HPP
