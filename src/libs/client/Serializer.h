/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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

#ifndef PATCHLIBRARIAN_H
#define PATCHLIBRARIAN_H

#include <map>
#include <utility>
#include <string>
#include <redland.h>
#include <cassert>
#include "util/CountedPtr.h"
#include "util/Path.h"
#include "util/Atom.h"
#include "ObjectModel.h"

using std::string;

namespace Ingen {
namespace Client {

class PatchModel;
class NodeModel;
class PortModel;
class ConnectionModel;
class PresetModel;
class ModelEngineInterface;


/** Namespace prefix macros. */
#define NS_RDF(x) "http://www.w3.org/1999/02/22-rdf-syntax-ns#" x
#define NS_INGEN(x) "http://codeson.net/ns/ingen#" x

	
/** Handles all patch saving and loading.
 *
 * \ingroup IngenClient
 */
class Serializer
{
public:
	Serializer(CountedPtr<ModelEngineInterface> engine);
	~Serializer();

	void          path(const string& path) { _patch_search_path = path; }
	const string& path()                   { return _patch_search_path; }
	
	string find_file(const string& filename, const string& additional_path = "");
	
	void save_patch(CountedPtr<PatchModel> patch_model, const string& filename, bool recursive);
	
	string load_patch(const string& filename,
	                  const string& parent_path,
	                  const string& name,
	                  size_t        poly,
	                  MetadataMap   initial_data,
	                  bool          existing = false);

private:

	// Model -> RDF
	
	void add_patch_to_rdf(librdf_model* rdf,
		CountedPtr<PatchModel> patch, const string& uri);
	
	void add_node_to_rdf(librdf_model* rdf,
		CountedPtr<NodeModel> node, const string ns_prefix="");
	
	void add_port_to_rdf(librdf_model* rdf,
		CountedPtr<PortModel> port, const string ns_prefix="");
	
	void add_connection_to_rdf(librdf_model* rdf,
		CountedPtr<ConnectionModel> connection, const string port_ns_prefix="");


	// Triple -> RDF
	
	void add_resource_to_rdf(librdf_model* model,
			const string& subject_uri, const string& predicate_uri, const string& object_uri);
	
	void add_resource_to_rdf(librdf_model* model,
			librdf_node* subject, const string& predicate_uri, const string& object_uri);
	
	void add_atom_to_rdf(librdf_model* model,
			const string& subject_uri, const string& predicate_uri, const Atom& atom);
	

	void   create();
	void   destroy();
	void   setup_prefixes();
	string expand_uri(const string& uri);


	librdf_world*                    _world;
	librdf_serializer*               _serializer;
	string                           _patch_search_path;
	map<string, string>              _prefixes;
	CountedPtr<ModelEngineInterface> _engine;
};


} // namespace Client
} // namespace Ingen

#endif // PATCHLIBRARIAN_H
