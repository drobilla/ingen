/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
 *
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef INGEN_SERIALISATION_SERIALISER_HPP
#define INGEN_SERIALISATION_SERIALISER_HPP

#include <stdexcept>
#include <string>

#include "raul/Path.hpp"
#include "raul/SharedPtr.hpp"

#include "sord/sordmm.hpp"

#include "ingen/GraphObject.hpp"

namespace Ingen {

class Plugin;
class GraphObject;
class Patch;
class Node;
class Port;
class Connection;

namespace Shared { class World; class Store; }

namespace Serialisation {

/** Serialises Ingen objects (patches, nodes, etc) to RDF.
 *
 * \ingroup IngenSerialisation
 */
class Serialiser
{
public:
	Serialiser(Shared::World& world, SharedPtr<Shared::Store> store);
	~Serialiser();

	typedef GraphObject::Properties Properties;

	void to_file(SharedPtr<const GraphObject> object,
	             const std::string&           filename);

	void write_bundle(SharedPtr<const Patch> patch,
	                  const std::string&     path);

	std::string to_string(SharedPtr<const GraphObject> object,
	                      const std::string&           base_uri,
	                      const Properties&            extra_rdf);

	void start_to_string(const Raul::Path&  root,
	                     const std::string& base_uri);

	void serialise(SharedPtr<const GraphObject> object) throw (std::logic_error);

	void serialise_connection(const Sord::Node& parent,
	                          SharedPtr<const Connection> c) throw (std::logic_error);

	std::string finish();

private:
	struct Impl;
	Impl* me;
};

} // namespace Serialisation
} // namespace Ingen

#endif // INGEN_SERIALISATION_SERIALISER_HPP
