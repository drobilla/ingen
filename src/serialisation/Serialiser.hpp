/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#ifndef SERIALISER_H
#define SERIALISER_H

#include <map>
#include <utility>
#include <string>
#include <stdexcept>
#include <cassert>
#include "raul/SharedPtr.hpp"
#include "raul/Path.hpp"
#include "raul/Atom.hpp"
#include "redlandmm/World.hpp"
#include "redlandmm/Model.hpp"
#include "interface/GraphObject.hpp"
#include "shared/Store.hpp"

using namespace Raul;
using namespace Ingen::Shared;

namespace Ingen {

namespace Shared {
	class Plugin;
	class GraphObject;
	class Patch;
	class Node;
	class Port;
	class Connection;
	class World;
}

namespace Serialisation {


/** Serialises Ingen objects (patches, nodes, etc) to RDF.
 *
 * \ingroup IngenClient
 */
class Serialiser
{
public:
	Serialiser(Shared::World& world, SharedPtr<Shared::Store> store);

	void to_file(SharedPtr<GraphObject> object, const std::string& filename);

	std::string to_string(SharedPtr<GraphObject>        object,
	                      const std::string&            base_uri,
	                      const GraphObject::Variables& extra_rdf);
	
	void start_to_string(const Raul::Path& root, const std::string& base_uri);
	void serialise(SharedPtr<GraphObject> object) throw (std::logic_error);
	void serialise_connection(SharedPtr<GraphObject>        parent,
	                          SharedPtr<Shared::Connection> c) throw (std::logic_error);
	
	std::string finish();
	
private:
	enum Mode { TO_FILE, TO_STRING };
	
	void start_to_filename(const std::string& filename);

	void setup_prefixes();

	void serialise_plugin(SharedPtr<Shared::Plugin> p);

	void serialise_patch(SharedPtr<Shared::Patch> p);
	void serialise_node(SharedPtr<Shared::Node> n, const Redland::Node& id);
	void serialise_port(const Shared::Port* p, const Redland::Node& id);

	void serialise_properties(Redland::Node subject, const GraphObject::Variables& properties);
	void serialise_variables(Redland::Node subject, const GraphObject::Variables& variables);
	
	Redland::Node path_to_rdf_node(const Path& path);
	Redland::Node patch_path_to_rdf_node(const Path& path);

	Raul::Path               _root_path;
	SharedPtr<Shared::Store> _store;
	Mode                     _mode;
	std::string              _base_uri;
	Redland::World&          _world;
	Redland::Model*          _model;

#ifdef USE_BLANK_NODES
	typedef std::map<Raul::Path, Redland::Node> NodeMap;
	NodeMap _node_map;
#endif
};


} // namespace Serialisation
} // namespace Ingen

#endif // SERIALISER_H
