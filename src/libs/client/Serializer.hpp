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

#ifndef SERIALIZER_H
#define SERIALIZER_H

#include <map>
#include <utility>
#include <string>
#include <stdexcept>
#include <cassert>
#include <raul/SharedPtr.hpp>
#include <raul/Path.hpp>
#include <raul/Atom.hpp>
#include <raul/RDFWorld.hpp>
#include <raul/RDFModel.hpp>
#include <raul/Table.hpp>
#include "ObjectModel.hpp"

namespace Ingen {
namespace Client {

class PluginModel;
class PatchModel;
class NodeModel;
class PortModel;
class ConnectionModel;
class PresetModel;


/** Serializes Ingen objects (patches, nodes, etc) to RDF.
 *
 * \ingroup IngenClient
 */
class Serializer
{
public:
	Serializer(Raul::RDF::World& world);

	void   start_to_filename(const string& filename);
	void   start_to_string();

	void   serialize(SharedPtr<ObjectModel> object)           throw (std::logic_error);
	void   serialize_connection(SharedPtr<ConnectionModel> c) throw (std::logic_error);
	
	string finish();

private:
	enum Mode { TO_FILE, TO_STRING };

	void setup_prefixes();

	void serialize_plugin(SharedPtr<PluginModel> p);

	void serialize_patch(SharedPtr<PatchModel> p, const Raul::RDF::Node& id);
	void serialize_node(SharedPtr<NodeModel> n, const Raul::RDF::Node& id);
	void serialize_port(SharedPtr<PortModel> p, const Raul::RDF::Node& id);
	
	Raul::RDF::Node path_to_node_id(const Path& path);

	typedef Raul::Table<Path, Raul::RDF::Node> NodeMap;

	Mode              _mode;
	NodeMap           _node_map;
	string            _base_uri;
	Raul::RDF::World& _world;
	Raul::RDF::Model* _model;
};


} // namespace Client
} // namespace Ingen

#endif // SERIALIZER_H
