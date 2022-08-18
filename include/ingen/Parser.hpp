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

#ifndef INGEN_PARSER_HPP
#define INGEN_PARSER_HPP

#include "ingen/FilePath.hpp"
#include "ingen/Properties.hpp" // IWYU pragma: keep
#include "ingen/URI.hpp"
#include "ingen/ingen.h"
#include "raul/Path.hpp" // IWYU pragma: keep
#include "raul/Symbol.hpp" // IWYU pragma: keep

#include <boost/optional/optional.hpp>

#include <set>
#include <string>
#include <utility>

namespace Sord {
class World;
} // namespace Sord

namespace ingen {

class Interface;
class World;

/**
   Parser for reading graphs from Turtle files or strings.

   @ingroup Ingen
*/
class INGEN_API Parser
{
public:
	explicit Parser() = default;

	virtual ~Parser() = default;

	/** Record of a resource listed in a bundle manifest. */
	struct ResourceRecord {
		ResourceRecord(URI u, FilePath f)
			: uri(std::move(u)), filename(std::move(f))
		{}

		bool operator<(const ResourceRecord& r) const {
			return uri < r.uri;
		}

		URI      uri;      ///< URI of resource (e.g. a Graph)
		FilePath filename; ///< Path of describing file (seeAlso)
	};

	/** Find all resources of a given type listed in a manifest file. */
	virtual std::set<ResourceRecord> find_resources(Sord::World& world,
	                                                const URI&   manifest_uri,
	                                                const URI&   type_uri);

	/** Parse a graph from RDF into a Interface (engine or client).
	 *
	 * If `path` is a file path, then the graph is loaded from that
	 * file.  If it is a directory, then the manifest.ttl from that directory
	 * is used instead.  In either case, any rdfs:seeAlso files are loaded and
	 * the graph parsed from the resulting combined model.
	 *
	 * @return whether or not load was successful.
	 */
	virtual bool parse_file(
		World&                               world,
		Interface&                           target,
		const FilePath&                      path,
		const boost::optional<raul::Path>&   parent = boost::optional<raul::Path>(),
		const boost::optional<raul::Symbol>& symbol = boost::optional<raul::Symbol>(),
		const boost::optional<Properties>&   data   = boost::optional<Properties>());

	virtual boost::optional<URI> parse_string(
		World&                               world,
		Interface&                           target,
		const std::string&                   str,
		const URI&                           base_uri,
		const boost::optional<raul::Path>&   parent = boost::optional<raul::Path>(),
		const boost::optional<raul::Symbol>& symbol = boost::optional<raul::Symbol>(),
		const boost::optional<Properties>&   data   = boost::optional<Properties>());
};

} // namespace ingen

#endif // INGEN_PARSER_HPP
