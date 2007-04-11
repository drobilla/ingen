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
#include <boost/optional/optional.hpp>
#include <raptor.h>
#include "raul/SharedPtr.h"
#include "raul/Path.h"
#include "raul/Atom.h"
#include "raul/RDFWriter.h"
#include "ObjectModel.h"

using std::string;
using boost::optional;
using Raul::RdfId;

namespace Ingen {
namespace Client {

class PluginModel;
class PatchModel;
class NodeModel;
class PortModel;
class ConnectionModel;
class PresetModel;
class ModelEngineInterface;


/** Namespace prefix macros. */
#define NS_RDF(x) RdfId(RdfId::RESOURCE, "http://www.w3.org/1999/02/22-rdf-syntax-ns#" x)
#define NS_INGEN(x) RdfId(RdfId::RESOURCE, "http://drobilla.net/ns/ingen#" x)

	
/** Serializes Ingen objects (patches, nodes, etc) to RDF.
 *
 * \ingroup IngenClient
 */
class Serializer
{
public:
	Serializer();

	void   start_to_filename(const string& filename)          throw (std::logic_error);
	void   start_to_string()                                  throw (std::logic_error);
	void   serialize(SharedPtr<ObjectModel> object)           throw (std::logic_error);
	void   serialize_connection(SharedPtr<ConnectionModel> c) throw (std::logic_error);
	string finish()                                           throw (std::logic_error);

private:

	void serialize_plugin(SharedPtr<PluginModel> p);

	void serialize_patch(SharedPtr<PatchModel> p, const Raul::RdfId& id);
	void serialize_node(SharedPtr<NodeModel> n, const Raul::RdfId& id);
	void serialize_port(SharedPtr<PortModel> p, const Raul::RdfId& id);
	
	Raul::RdfId path_to_node_id(const Path& path);

	typedef std::map<Path, RdfId> IDMap;
	IDMap           _id_map;
	string          _base_uri;
	Raul::RDFWriter _writer;
};


} // namespace Client
} // namespace Ingen

#endif // SERIALIZER_H
