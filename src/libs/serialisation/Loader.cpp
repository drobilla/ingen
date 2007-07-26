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

#include <iostream>
#include <locale.h>
#include <glibmm/ustring.h>
#include <raul/RDFModel.hpp>
#include <raul/RDFQuery.hpp>
#include <raul/TableImpl.hpp>
#include "interface/EngineInterface.hpp"
#include "Loader.hpp"

using namespace std;
using namespace Raul;
using namespace Ingen::Shared;

namespace Ingen {
namespace Serialisation {


/** Load (create) a patch from RDF into the engine.
 *
 * @param document_uri URI of file to load objects from.
 * @param parent Path of parent under which to load objects.
 * @return whether or not load was successful.
 */
bool
Loader::load(SharedPtr<EngineInterface> engine,
             Raul::RDF::World*          rdf_world,
             const Glib::ustring&       document_uri,
             boost::optional<Path>      parent,
             string                     patch_name,
             Glib::ustring              patch_uri,
             Raul::Table<string, Atom>  data)
{
	setlocale(LC_NUMERIC, "C");

	// FIXME: this whole thing is a mess
	
	Raul::Table<Path, bool> created;

	RDF::Model model(*rdf_world, document_uri);

	if (patch_uri == "")
		patch_uri = string("<") + document_uri + ">";
	else
		patch_uri = string("<") + patch_uri + ">";

	cerr << "[Loader] Loading " << patch_uri << " from " << document_uri
		<< " under " << (string)(parent ? (string)parent.get() : "no parent") << endl;

	/* Get polyphony (mandatory) */

	// FIXME: polyphony datatype
	RDF::Query query(*rdf_world, Glib::ustring(
		"SELECT DISTINCT ?poly WHERE {\n") +
		patch_uri + " ingen:polyphony ?poly\n }");

	RDF::Query::Results results = query.run(*rdf_world, model);

	if (results.size() == 0) {
		cerr << "[Loader] ERROR: No polyphony found!" << endl;
		return false;
	}

	RDF::Node poly_node = (*results.begin())["poly"];
	assert(poly_node.is_int());
	const size_t patch_poly = static_cast<size_t>(poly_node.to_int());
	
	/* Get name (if available/necessary) */

	if (patch_name == "") {	
		patch_name = string(document_uri.substr(document_uri.find_last_of("/")+1));
		if (patch_name.substr(patch_name.length()-10) == ".ingen.ttl")
			patch_name = patch_name.substr(0, patch_name.length()-10);
		
		query = RDF::Query(*rdf_world, Glib::ustring(
			"SELECT DISTINCT ?name WHERE {\n") +
			patch_uri + " ingen:name ?name\n}");

		results = query.run(*rdf_world, model);

		if (results.size() > 0)
			patch_name = (*results.begin())["name"].to_string();
	}

	Path patch_path = ( parent ? (parent.get().base() + patch_name) : Path("/") );
	cerr << "************ PATCH: name=" << patch_name << ", path=" << patch_path
		<< ", poly = " << patch_poly << endl;
	engine->create_patch(patch_path, patch_poly);


	/* Load (plugin) nodes */
	
	query = RDF::Query(*rdf_world, Glib::ustring(
		"SELECT DISTINCT ?name ?plugin ?floatkey ?floatval WHERE {\n") +
		patch_uri + " ingen:node   ?node .\n"
		"?node        ingen:name   ?name ;\n"
		"             ingen:plugin ?plugin .\n"
		"OPTIONAL { ?node ?floatkey ?floatval . \n"
		"           FILTER ( datatype(?floatval) = xsd:decimal ) }\n"
		"}");

	results = query.run(*rdf_world, model);

	for (RDF::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		
		const string name   = (*i)["name"].to_string();
		const string plugin = (*i)["plugin"].to_string();
		
		const Path node_path = patch_path.base() + (string)name;

		if (created.find(node_path) == created.end()) {
			engine->create_node(node_path, plugin, false);
			created[node_path] = true;
		}

		string floatkey = rdf_world->prefixes().qualify((*i)["floatkey"].to_string());
		RDF::Node val_node = (*i)["floatval"];

		if (floatkey != "" && val_node.is_float())
			engine->set_metadata(patch_path.base() + name, floatkey, Atom(val_node.to_float()));
	}
	

	/* Load subpatches */
	
	query = RDF::Query(*rdf_world, Glib::ustring(
		"SELECT DISTINCT ?patch ?name WHERE {\n") +
		patch_uri + " ingen:node ?patch .\n"
		"?patch       a          ingen:Patch ;\n"
		"             ingen:name ?name .\n"
		"}");

	results = query.run(*rdf_world, model);

	for (RDF::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		
		const string name  = (*i)["name"].to_string();
		const string patch = (*i)["patch"].to_string();
		
		const Path subpatch_path = patch_path.base() + (string)name;
		
		if (created.find(subpatch_path) == created.end()) {
			created[subpatch_path] = true;
			load(engine, rdf_world, document_uri, patch_path, name, patch);
		}
	}
	
	//created.clear();


	/* Set node port control values */
	
	query = RDF::Query(*rdf_world, Glib::ustring(
		"SELECT DISTINCT ?nodename ?portname ?portval WHERE {\n") +
		patch_uri + " ingen:node   ?node .\n"
		"?node        ingen:name   ?nodename ;\n"
		"             ingen:port   ?port .\n"
		"?port        ingen:name   ?portname ;\n"
		"             ingen:value  ?portval .\n"
		"FILTER ( datatype(?portval) = xsd:decimal )\n"
		"}\n");

	results = query.run(*rdf_world, model);

	for (RDF::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		
		const string node_name = (*i)["nodename"].to_string();
		const string port_name = (*i)["portname"].to_string();
		const float  val       = (*i)["portval"].to_float();

		const Path port_path = patch_path.base() + node_name +"/"+ port_name;

		engine->set_port_value(port_path, val);
	}
	

	/* Load this patch's ports */
	
	query = RDF::Query(*rdf_world, Glib::ustring(
		"SELECT DISTINCT ?port ?type ?name ?datatype ?floatkey ?floatval ?portval WHERE {\n") +
		patch_uri + " ingen:port     ?port .\n"
		"?port        a              ?type ;\n"
		"             ingen:name     ?name ;\n"
		"             ingen:dataType ?datatype .\n"
		"OPTIONAL { ?port ?floatkey ?floatval . \n"
		"           FILTER ( datatype(?floatval) = xsd:decimal ) }\n"
		"OPTIONAL { ?port ingen:value ?portval . \n"
		"           FILTER ( datatype(?portval) = xsd:decimal ) }\n"
		"}");

	results = query.run(*rdf_world, model);

	for (RDF::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		const string name     = (*i)["name"].to_string();
		const string type     = rdf_world->qualify((*i)["type"].to_string());
		const string datatype = (*i)["datatype"].to_string();

		const Path port_path = patch_path.base() + (string)name;

		if (created.find(port_path) == created.end()) {
			//cerr << "TYPE: " << type << endl;
			bool is_output = (type == "ingen:OutputPort"); // FIXME: check validity
			engine->create_port(port_path, datatype, is_output);
			created[port_path] = true;
		}

		RDF::Node val_node = (*i)["portval"];
		if (val_node.is_float())
			engine->set_port_value(patch_path.base() + name, val_node.to_float());

		string floatkey = rdf_world->qualify((*i)["floatkey"].to_string());
		val_node = (*i)["floatval"];

		if (floatkey != "" && val_node.is_float())
			engine->set_metadata(patch_path.base() + name, floatkey, Atom(val_node.to_float()));
	}
	
	created.clear();


	/* Node -> Node connections */

	query = RDF::Query(*rdf_world, Glib::ustring(
		"SELECT DISTINCT ?srcnodename ?srcname ?dstnodename ?dstname WHERE {\n") +
		patch_uri +  "ingen:node ?srcnode ;\n"
		"             ingen:node ?dstnode .\n"
		"?srcnode     ingen:port ?src ;\n"
		"             ingen:name ?srcnodename .\n"
		"?dstnode     ingen:port ?dst ;\n"
		"             ingen:name ?dstnodename .\n"
		"?src         ingen:name ?srcname .\n"
		"?dst         ingen:connectedTo ?src ;\n"
		"             ingen:name ?dstname .\n" 
		"}\n");
	
	results = query.run(*rdf_world, model);
	
	for (RDF::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		Path src_node = patch_path.base() + (*i)["srcnodename"].to_string();
		Path src_port = src_node.base() + (*i)["srcname"].to_string();
		Path dst_node = patch_path.base() + (*i)["dstnodename"].to_string();
		Path dst_port = dst_node.base() + (*i)["dstname"].to_string();
		
		cerr << patch_path << " 1 CONNECTION: " << src_port << " -> " << dst_port << endl;

		engine->connect(src_port, dst_port);
	}
	

	/* This Patch -> Node connections */

	query = RDF::Query(*rdf_world, Glib::ustring(
		"SELECT DISTINCT ?srcname ?dstnodename ?dstname WHERE {\n") +
		patch_uri + " ingen:port ?src ;\n"
		"             ingen:node ?dstnode .\n"
		"?dstnode     ingen:port ?dst ;\n"
		"             ingen:name ?dstnodename .\n"
		"?dst         ingen:connectedTo ?src ;\n"
		"             ingen:name ?dstname .\n" 
		"?src         ingen:name ?srcname .\n"
		"}\n");
	
	results = query.run(*rdf_world, model);
	
	for (RDF::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		Path src_port = patch_path.base() + (*i)["srcname"].to_string();
		Path dst_node = patch_path.base() + (*i)["dstnodename"].to_string();
		Path dst_port = dst_node.base() + (*i)["dstname"].to_string();
		
		cerr << patch_path << " 2 CONNECTION: " << src_port << " -> " << dst_port << endl;

		engine->connect(src_port, dst_port);
	}
	
	
	/* Node -> This Patch connections */

	query = RDF::Query(*rdf_world, Glib::ustring(
		"SELECT DISTINCT ?srcnodename ?srcname ?dstname WHERE {\n") +
		patch_uri + " ingen:port ?dst ;\n"
		"             ingen:node ?srcnode .\n"
		"?srcnode     ingen:port ?src ;\n"
		"             ingen:name ?srcnodename .\n"
		"?dst         ingen:connectedTo ?src ;\n"
		"             ingen:name ?dstname .\n" 
		"?src         ingen:name ?srcname .\n"
		"}\n");
	
	results = query.run(*rdf_world, model);
	
	for (RDF::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		Path dst_port = patch_path.base() + (*i)["dstname"].to_string();
		Path src_node = patch_path.base() + (*i)["srcnodename"].to_string();
		Path src_port = src_node.base() + (*i)["srcname"].to_string();
		
		cerr << patch_path << " 3 CONNECTION: " << src_port << " -> " << dst_port << endl;

		engine->connect(src_port, dst_port);
	}
	
	
	/* Load metadata */
	
	query = RDF::Query(*rdf_world, Glib::ustring(
		"SELECT DISTINCT ?floatkey ?floatval WHERE {\n") +
		patch_uri + " ?floatkey ?floatval . \n"
		"           FILTER ( datatype(?floatval) = xsd:decimal ) \n"
		"}");

	results = query.run(*rdf_world, model);

	for (RDF::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {

		string floatkey = rdf_world->prefixes().qualify((*i)["floatkey"].to_string());
		RDF::Node val_node = (*i)["floatval"];

		if (floatkey != "" && val_node.is_float())
			engine->set_metadata(patch_path, floatkey, Atom(val_node.to_float()));
	}
	

	// Set passed metadata last to override any loaded values
	for (Metadata::const_iterator i = data.begin(); i != data.end(); ++i)
		engine->set_metadata(patch_path, i->first, i->second);

	return true;
}


} // namespace Serialisation
} // namespace Ingen

