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

#ifndef INGEN_GRAPHOBJECT_HPP
#define INGEN_GRAPHOBJECT_HPP

#include "raul/Path.hpp"
#include "raul/SharedPtr.hpp"
#include "ingen/Resource.hpp"

namespace Raul {
class Atom;
class Path;
class Symbol;
}

namespace Ingen {

class Edge;
class Plugin;
class Store;

/** An object on the audio graph - Graph, Block, Port, etc.
 *
 * @ingroup Ingen
 */
class GraphObject : public Resource
{
public:
	enum GraphType {
		GRAPH,
		BLOCK,
		PORT
	};

	typedef std::pair<const GraphObject*, const GraphObject*> EdgesKey;
	typedef std::map< EdgesKey, SharedPtr<Edge> > Edges;

	// Graphs only
	Edges&       edges()       { return _edges; }
	const Edges& edges() const { return _edges; }

	// Blocks and graphs only
	virtual uint32_t      num_ports()          const { return 0; }
	virtual GraphObject*  port(uint32_t index) const { return NULL; }
	virtual const Plugin* plugin()             const { return NULL; }

	// All objects
	virtual GraphType           graph_type()   const = 0;
	virtual const Raul::Path&   path()         const = 0;
	virtual const Raul::Symbol& symbol()       const = 0;
	virtual GraphObject*        graph_parent() const = 0;

	static Raul::URI root_uri() { return Raul::URI("ingen:root"); }

	static bool uri_is_path(const Raul::URI& uri) {
		return uri.substr(0, root_uri().length()) == root_uri();
	}

	static Raul::Path uri_to_path(const Raul::URI& uri) {
		return (uri == root_uri())
			? Raul::Path("/")
			: Raul::Path(uri.substr(root_uri().length()));
	}

	static Raul::URI path_to_uri(const Raul::Path& path) {
		return Raul::URI(root_uri() + path.c_str());
	}

protected:
	friend class Store;
	virtual void set_path(const Raul::Path& p) = 0;

	GraphObject(URIs& uris, const Raul::Path& path)
		: Resource(uris, path_to_uri(path))
	{}

	Edges _edges;  ///< Graphs only
};

} // namespace Ingen

#endif // INGEN_GRAPHOBJECT_HPP
