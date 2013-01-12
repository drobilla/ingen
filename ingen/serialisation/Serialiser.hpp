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

#ifndef INGEN_SERIALISATION_SERIALISER_HPP
#define INGEN_SERIALISATION_SERIALISER_HPP

#include <stdexcept>
#include <string>

#include "raul/Path.hpp"

#include "sord/sordmm.hpp"

#include "ingen/Node.hpp"

namespace Ingen {

class Arc;
class Node;
class Store;
class World;

namespace Serialisation {

/**
   Write Ingen objects to Turtle files or strings.

   @ingroup IngenSerialisation
*/
class Serialiser
{
public:
	explicit Serialiser(World& world);
	virtual ~Serialiser();

	typedef Node::Properties Properties;

	virtual void to_file(SPtr<const Node>   object,
	                     const std::string& filename);

	virtual void write_bundle(SPtr<const Node>   graph,
	                          const std::string& path);

	virtual std::string to_string(SPtr<const Node>   object,
	                              const std::string& base_uri);

	virtual void start_to_string(const Raul::Path&  root,
	                             const std::string& base_uri);

	virtual void serialise(SPtr<const Node> object)
			throw (std::logic_error);

	virtual void serialise_arc(const Sord::Node& parent,
	                           SPtr<const Arc>   arc)
			throw (std::logic_error);

	virtual std::string finish();

private:
	struct Impl;
	Impl* me;
};

} // namespace Serialisation
} // namespace Ingen

#endif // INGEN_SERIALISATION_SERIALISER_HPP
