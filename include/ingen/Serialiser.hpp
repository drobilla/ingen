/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_SERIALISER_HPP
#define INGEN_SERIALISER_HPP

#include "ingen/FilePath.hpp"
#include "ingen/Properties.hpp"
#include "ingen/ingen.h"
#include "sord/sordmm.hpp"

#include <memory>
#include <string>

namespace raul { class Path; }

namespace ingen {

class Arc;
class Node;
class URI;
class World;

/**
   Serialiser for writing graphs to Turtle files or strings.

   @ingroup Ingen
*/
class INGEN_API Serialiser
{
public:
	explicit Serialiser(World& world);

	virtual ~Serialiser();

	/** Write a graph and all its contents as a complete bundle. */
	virtual void
	write_bundle(const std::shared_ptr<const Node>& graph, const URI& uri);

	/** Begin a serialization to a string.
	 *
	 * This must be called before any serializing methods.
	 *
	 * The results of the serialization will be returned by the finish() method after
	 * the desired objects have been serialised.
	 *
	 * All serialized paths will have the root path chopped from their prefix
	 * (therefore all serialized paths must be descendants of the root)
	 */
	virtual void start_to_string(const raul::Path& root,
	                             const URI&        base_uri);

	/** Begin a serialization to a file.
	 *
	 * This must be called before any serializing methods.
	 *
	 * All serialized paths will have the root path chopped from their prefix
	 * (therefore all serialized paths must be descendants of the root)
	 */
	virtual void start_to_file(const raul::Path& root,
	                           const FilePath&   filename);

	/** Serialize an object (graph, block, or port).
	 *
	 * @throw std::logic_error
	 */
	virtual void serialise(const std::shared_ptr<const Node>& object,
	                       Property::Graph context = Property::Graph::DEFAULT);

	/** Serialize an arc.
	 *
	 * @throw std::logic_error
	 */
	virtual void serialise_arc(const Sord::Node&                 parent,
	                           const std::shared_ptr<const Arc>& arc);

	/** Finish serialization.
	 *
	 * If this is a file serialization, this must be called to finish and close
	 * the output file, and the empty string is returned.
	 *
	 * If this is a string serialization, the serialized result is returned.
	 */
	virtual std::string finish();

private:
	struct Impl;

	std::unique_ptr<Impl> me;
};

} // namespace ingen

#endif // INGEN_SERIALISER_HPP
