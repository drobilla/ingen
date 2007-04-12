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
#include <glibmm/ustring.h>
#include <raptor.h>
#include "raul/RDFQuery.h"
#include "Loader.h"
#include "ModelEngineInterface.h"

using namespace Raul;

namespace Ingen {
namespace Client {


Loader::Loader(SharedPtr<ModelEngineInterface> engine, SharedPtr<Namespaces> namespaces)
	: _engine(engine)
	, _namespaces(namespaces)
{
	if (!_namespaces)
		_namespaces = SharedPtr<Namespaces>(new Namespaces());

	(*_namespaces)["xsd"] = "http://www.w3.org/2001/XMLSchema#";
	(*_namespaces)["ingen"] = "http://drobilla.net/ns/ingen#";
	(*_namespaces)["ingenuity"] = "http://drobilla.net/ns/ingenuity#";
	(*_namespaces)["lv2"] = "http://lv2plug.in/ontology#";
}


/** Load (create) all objects from an RDF into the engine.
 *
 * @param document_uri URI of file to load objects from.
 * @param parent Path of parent under which to load objects.
 * @return whether or not load was successful.
 */
bool
Loader::load(const Glib::ustring&  document_uri,
             boost::optional<Path> parent,
			 string                patch_name,
			 Glib::ustring         patch_uri,
			 MetadataMap           data)
{
	// FIXME: this whole thing is a mess
	
	std::map<Path, bool> created;

	// FIXME: kluge
	//unsigned char* document_uri_str = raptor_uri_filename_to_uri_string(filename.c_str());
	//Glib::ustring document_uri = (const char*)document_uri_str;
	//Glib::ustring document_uri = "file:///home/dave/code/drobillanet/ingen/src/progs/ingenuity/test2.ingen.ttl";

	patch_uri = string("<") + patch_uri + ">";

	cerr << "[Loader] Loading " << patch_uri << " from " << document_uri
		<< " under " << (string)(parent ? (string)parent.get() : "no parent") << endl;

	/* Get polyphony (mandatory) */

	// FIXME: polyphony datatype
	RDFQuery query(*_namespaces, Glib::ustring(
		"SELECT DISTINCT ?poly FROM <") + document_uri + "> WHERE {\n" +
		patch_uri + " ingen:polyphony ?poly .\n"
		"}");

	RDFQuery::Results results = query.run(document_uri);

	if (results.size() == 0) {
		cerr << "[Loader] ERROR: No polyphony found!" << endl;
		return false;
	}

	size_t patch_poly = atoi(((*results.begin())["poly"]).c_str());
	
	
	/* Get name (if available/necessary) */

	if (patch_name == "") {	
		patch_name = string(document_uri.substr(document_uri.find_last_of("/")+1));
		if (patch_name.substr(patch_name.length()-10) == ".ingen.ttl")
			patch_name = patch_name.substr(0, patch_name.length()-10);
		
		query = RDFQuery(*_namespaces, Glib::ustring(
			"SELECT DISTINCT ?name FROM <") + document_uri + "> WHERE {\n" +
			patch_uri + " ingen:name ?name .\n"
			"}");

		results = query.run(document_uri);

		if (results.size() > 0)
			patch_name = string((*results.begin())["name"]);
	}

	Path patch_path = ( parent ? (parent.get().base() + patch_name) : Path("/") );
	cerr << "************ PATCH: name=" << patch_name << ", path=" << patch_path
		<< ", poly = " << patch_poly << endl;
	_engine->create_patch(patch_path, patch_poly);


	/* Load (plugin) nodes */
	
	query = RDFQuery(*_namespaces, Glib::ustring(
		"SELECT DISTINCT ?name ?plugin ?floatkey ?floatval FROM <") + document_uri + "> WHERE {\n" +
		patch_uri + " ingen:node   ?node .\n"
		"?node        ingen:name   ?name ;\n"
		"             ingen:plugin ?plugin .\n"
		"OPTIONAL { ?node ?floatkey ?floatval . \n"
		"           FILTER ( datatype(?floatval) = xsd:decimal ) }\n"
		"}");

	results = query.run(document_uri);

	for (RDFQuery::Results::iterator i = results.begin(); i != results.end(); ++i) {
		
		const Glib::ustring& name   = (*i)["name"];
		const Glib::ustring& plugin = (*i)["plugin"];
		
		const Path node_path = patch_path.base() + (string)name;

		if (created.find(node_path) == created.end()) {
			_engine->create_node(node_path, plugin, false);
			created[node_path] = true;
		}

		const Glib::ustring& floatkey = _namespaces->qualify((*i)["floatkey"]);
		const Glib::ustring& floatval = (*i)["floatval"];

		if (floatkey != "" && floatval != "") {
			const float val = atof(floatval.c_str());
			_engine->set_metadata(patch_path.base() + name, floatkey, Atom(val));
		}
	}
	

	/* Load subpatches */
	
	query = RDFQuery(*_namespaces, Glib::ustring(
		"SELECT DISTINCT ?patch ?name FROM <") + document_uri + "> WHERE {\n" +
		patch_uri + " ingen:node ?patch .\n"
		"?patch       a          ingen:Patch ;\n"
		"             ingen:name ?name .\n"
		"}");

	results = query.run(document_uri);

	for (RDFQuery::Results::iterator i = results.begin(); i != results.end(); ++i) {
		
		const Glib::ustring& name  = (*i)["name"];
		const Glib::ustring& patch = (*i)["patch"];
		
		const Path subpatch_path = patch_path.base() + (string)name;
		
		if (created.find(subpatch_path) == created.end()) {
			load(document_uri, patch_path, name, patch);
			created[subpatch_path] = true;
		}
	}
	
	created.clear();


	/* Set node port control values */
	
	query = RDFQuery(*_namespaces, Glib::ustring(
		"SELECT DISTINCT ?nodename ?portname ?portval FROM <") + document_uri + "> WHERE {\n" +
		patch_uri + " ingen:node   ?node .\n"
		"?node        ingen:name   ?nodename ;\n"
		"             ingen:port   ?port .\n"
		"?port        ingen:name   ?portname ;\n"
		"             ingen:value  ?portval .\n"
		"}\n");

	results = query.run(document_uri);

	for (RDFQuery::Results::iterator i = results.begin(); i != results.end(); ++i) {
		
		const Glib::ustring& node_name = (*i)["nodename"];
		const Glib::ustring& port_name = (*i)["portname"];
		const Glib::ustring& portval   = (*i)["portval"];

		Path port_path = patch_path.base() + (const string&)node_name +"/"+ (const string&)port_name;

		if (portval != "") {
			const float val = atof(portval.c_str());
			cerr << port_path << " VALUE: " << val << endl;
			_engine->set_port_value(port_path, val);
		}
	}
	

	/* Load this patch's ports */
	
	query = RDFQuery(*_namespaces, Glib::ustring(
		"SELECT DISTINCT ?port ?type ?name ?datatype ?floatkey ?floatval ?portval FROM <") + document_uri + "> WHERE {\n" +
		patch_uri + " ingen:port     ?port .\n"
		"?port        a              ?type ;\n"
		"             ingen:name     ?name ;\n"
		"             ingen:dataType ?datatype .\n"
		"OPTIONAL { ?port ?floatkey ?floatval . \n"
		"           FILTER ( datatype(?floatval) = xsd:decimal ) }\n"
		"OPTIONAL { ?port ingen:value ?portval . \n"
		"           FILTER ( datatype(?portval) = xsd:decimal ) }\n"
		"}");

	results = query.run(document_uri);

	for (RDFQuery::Results::iterator i = results.begin(); i != results.end(); ++i) {
		const Glib::ustring& name     = (*i)["name"];
		const Glib::ustring& type     = _namespaces->qualify((*i)["type"]);
		const Glib::ustring& datatype = (*i)["datatype"];

		const Path port_path = patch_path.base() + (string)name;

		if (created.find(port_path) == created.end()) {
			//cerr << "TYPE: " << type << endl;
			bool is_output = (type == "ingen:OutputPort"); // FIXME: check validity
			_engine->create_port(port_path, datatype, is_output);
			created[port_path] = true;
		}

		const Glib::ustring& portval = (*i)["portval"];
		if (portval != "") {
			const float val = atof(portval.c_str());
			cerr << name << " VALUE: " << val << endl;
			_engine->set_port_value(patch_path.base() + name, val);
		}

		const Glib::ustring& floatkey = _namespaces->qualify((*i)["floatkey"]);
		const Glib::ustring& floatval = (*i)["floatval"];

		if (floatkey != "" && floatval != "") {
			const float val = atof(floatval.c_str());
			_engine->set_metadata(patch_path.base() + name, floatkey, Atom(val));
		}
	}
	
	created.clear();


	/* Node -> Node connections */

	query = RDFQuery(*_namespaces, Glib::ustring(
		"SELECT DISTINCT ?srcnodename ?srcname ?dstnodename ?dstname FROM <") + document_uri + "> WHERE {\n" +
		patch_uri + " ingen:node ?srcnode ;\n"
		"             ingen:node ?dstnode .\n"
		"?srcnode     ingen:port ?src ;\n"
		"             ingen:name ?srcnodename .\n" + 
		"?dstnode     ingen:port ?dst ;\n"
		"             ingen:name ?dstnodename .\n"
		"?src         ingen:name ?srcname .\n"
		"?dst         ingen:connectedTo ?src ;\n"
		"             ingen:name ?dstname .\n" 
		"}\n");
	
	results = query.run(document_uri);
	
	for (RDFQuery::Results::iterator i = results.begin(); i != results.end(); ++i) {
		Path src_node = patch_path.base() + string((*i)["srcnodename"]);
		Path src_port = src_node.base() + string((*i)["srcname"]);
		Path dst_node = patch_path.base() + string((*i)["dstnodename"]);
		Path dst_port = dst_node.base() + string((*i)["dstname"]);
		
		cerr << patch_path << " 1 CONNECTION: " << src_port << " -> " << dst_port << endl;

		_engine->connect(src_port, dst_port);
	}
	

	/* This Patch -> Node connections */

	query = RDFQuery(*_namespaces, Glib::ustring(
		"SELECT DISTINCT ?srcname ?dstnodename ?dstname FROM <") + document_uri + "> WHERE {\n" +
		patch_uri + " ingen:port ?src ;\n" +
		"             ingen:node ?dstnode .\n"
		"?dstnode     ingen:port ?dst ;\n"
		"             ingen:name ?dstnodename .\n"
		"?dst         ingen:connectedTo ?src ;\n"
		"             ingen:name ?dstname .\n" 
		"?src         ingen:name ?srcname .\n"
		"}\n");
	
	results = query.run(document_uri);
	
	for (RDFQuery::Results::iterator i = results.begin(); i != results.end(); ++i) {
		Path src_port = patch_path.base() + string((*i)["srcname"]);
		
		Path dst_node = patch_path.base() + string((*i)["dstnodename"]);
		Path dst_port = dst_node.base() + string((*i)["dstname"]);
		
		cerr << patch_path << " 2 CONNECTION: " << src_port << " -> " << dst_port << endl;

		_engine->connect(src_port, dst_port);
	}
	
	
	/* Node -> This Patch connections */

	query = RDFQuery(*_namespaces, Glib::ustring(
		"SELECT DISTINCT ?srcnodename ?srcname ?dstname FROM <") + document_uri + "> WHERE {\n" +
		patch_uri + " ingen:port ?dst ;\n" +
		"             ingen:node ?srcnode .\n"
		"?srcnode     ingen:port ?src ;\n"
		"             ingen:name ?srcnodename .\n"
		"?dst         ingen:connectedTo ?src ;\n"
		"             ingen:name ?dstname .\n" 
		"?src         ingen:name ?srcname .\n"
		"}\n");
	
	results = query.run(document_uri);
	
	for (RDFQuery::Results::iterator i = results.begin(); i != results.end(); ++i) {
		Path dst_port = patch_path.base() + string((*i)["dstname"]);
		
		Path src_node = patch_path.base() + string((*i)["srcnodename"]);
		Path src_port = src_node.base() + string((*i)["srcname"]);
		
		cerr << patch_path << " 3 CONNECTION: " << src_port << " -> " << dst_port << endl;

		_engine->connect(src_port, dst_port);
	}
	
	
	/* Load metadata */
	
	query = RDFQuery(*_namespaces, Glib::ustring(
		"SELECT DISTINCT ?floatkey ?floatval FROM <") + document_uri + "> WHERE {\n" +
		patch_uri + " ?floatkey ?floatval . \n"
		"           FILTER ( datatype(?floatval) = xsd:decimal ) \n"
		"}");

	results = query.run(document_uri);

	for (RDFQuery::Results::iterator i = results.begin(); i != results.end(); ++i) {

		const Glib::ustring& floatkey = _namespaces->qualify((*i)["floatkey"]);
		const Glib::ustring& floatval = (*i)["floatval"];

		if (floatkey != "" && floatval != "") {
			const float val = atof(floatval.c_str());
			_engine->set_metadata(patch_path, floatkey, Atom(val));
		}
	}
	

	// Set passed metadata last to override any loaded values
	for (MetadataMap::const_iterator i = data.begin(); i != data.end(); ++i)
		_engine->set_metadata(patch_path, i->first, i->second);

	return true;
}


} // namespace Client
} // namespace Ingen

