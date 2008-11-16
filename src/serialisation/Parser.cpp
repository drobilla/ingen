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
#include <set>
#include <locale.h>
#include <glibmm/ustring.h>
#include "redlandmm/Model.hpp"
#include "redlandmm/Node.hpp"
#include "redlandmm/Query.hpp"
#include "raul/TableImpl.hpp"
#include "raul/Atom.hpp"
#include "raul/AtomRDF.hpp"
#include "interface/EngineInterface.hpp"
#include "Parser.hpp"

using namespace std;
using namespace Raul;
using namespace Ingen::Shared;

namespace Ingen {
namespace Serialisation {

#define NS_INGEN "http://drobilla.net/ns/ingen#"
#define NS_LV2   "http://lv2plug.in/ns/lv2core#"
#define NS_LV2EV "http://lv2plug.in/ns/ext/event#"

Glib::ustring
Parser::uri_relative_to_base(Glib::ustring base, const Glib::ustring uri)
{
	//cout << "BASE: " << base << endl;
	base = base.substr(0, base.find_last_of("/")+1);
	//cout << uri << " RELATIVE TO " << base << endl;
	Glib::ustring ret;
	if (uri.length() >= base.length() && uri.substr(0, base.length()) == base)
		ret = uri.substr(base.length());
	else
		ret = uri;
	//cout << " => " << ret << endl;
	return ret;
}


/** Parse a patch from RDF into a CommonInterface (engine or client).
 * @return whether or not load was successful.
 */
bool
Parser::parse_document(
		Ingen::Shared::World*                   world,
		Ingen::Shared::CommonInterface*         target,
		const Glib::ustring&                    document_uri,
		Glib::ustring                           object_uri,
		Glib::ustring                           engine_base,
		boost::optional<Raul::Symbol>           symbol,
		boost::optional<GraphObject::Variables> data)
{
	Redland::Model model(*world->rdf_world, document_uri, document_uri);

	if (object_uri == document_uri || object_uri == "")
		cout << "Parsing document " << object_uri << " (base " << document_uri << ")" << endl;
	else
		cout << "Parsing " << object_uri << " from " << document_uri << endl;

	return parse(world, target, model, document_uri, engine_base, object_uri, symbol, data);;
}


bool
Parser::parse_string(
		Ingen::Shared::World*                   world,
		Ingen::Shared::CommonInterface*         target,
		const Glib::ustring&                    str,
		const Glib::ustring&                    base_uri,
		Glib::ustring                           engine_base,
		boost::optional<Glib::ustring>          object_uri,
		boost::optional<Raul::Symbol>           symbol,
		boost::optional<GraphObject::Variables> data)
{
	Redland::Model model(*world->rdf_world, str.c_str(), str.length(), base_uri);
	
	if (object_uri)
		cout << "Parsing " << object_uri.get() << " (base " << base_uri << ")" <<  endl;
	//else
	//	cout << "Parsing all objects found in string (base " << base_uri << ")" << endl;

	bool ret = parse(world, target, model, base_uri, engine_base, object_uri, symbol, data);
	if (ret) {
		const Glib::ustring subject = Glib::ustring("<") + base_uri + Glib::ustring(">");
		parse_connections(world, target, model, base_uri, subject,
				Path((engine_base == "") ? "/" : engine_base));
	}

	return ret;
}


bool
Parser::parse(
		Ingen::Shared::World*                   world,
		Ingen::Shared::CommonInterface*         target,
		Redland::Model&                         model,
		Glib::ustring                           base_uri,
		Glib::ustring                           engine_base,
		boost::optional<Glib::ustring>          object_uri,
		boost::optional<Raul::Symbol>           symbol,
		boost::optional<GraphObject::Variables> data)
{
	const Redland::Node::Type res = Redland::Node::RESOURCE;
	Glib::ustring query_str;
	if (object_uri && object_uri.get()[0] == '/')
		object_uri = object_uri.get().substr(1);
	

	/* **** First query out global information (top-level info) **** */
		
	// Delete anything explicitly declared to not exist
	query_str = Glib::ustring("SELECT DISTINCT ?o WHERE { ?o a owl:Nothing }");
	Redland::Query query(*world->rdf_world, query_str);
	Redland::Query::Results results = query.run(*world->rdf_world, model, base_uri);
	
	for (Redland::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		const Redland::Node& object = (*i)["o"];
		target->destroy(object.to_string());
	}
	
	// Variable settings
	query = Redland::Query(*world->rdf_world,
		"SELECT DISTINCT ?path ?varkey ?varval WHERE {\n"
		"?path     lv2var:variable ?variable .\n"
		"?variable rdf:predicate   ?varkey ;\n"
		"          rdf:value       ?varval .\n"
		"}");
	
	results = Redland::Query::Results(query.run(*world->rdf_world, model, base_uri));
	world->rdf_world->mutex().lock();
	for (Redland::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		const string obj_path = (*i)["path"].to_string();
		const string key = world->rdf_world->prefixes().qualify((*i)["varkey"].to_string());
		const Redland::Node& val_node = (*i)["varval"];
		if (key != "")
			target->set_variable(obj_path, key, AtomRDF::node_to_atom(val_node));
	}
	world->rdf_world->mutex().unlock();

	// Connections
	parse_connections(world, target, model, base_uri, "", "/");
	
	// Port values
	query = Redland::Query(*world->rdf_world,
		"SELECT DISTINCT ?path ?value WHERE {\n"
		"?path ingen:value ?value .\n"
		"}");
	
	results = Redland::Query::Results(query.run(*world->rdf_world, model, base_uri));
	world->rdf_world->mutex().lock();
	for (Redland::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		const string obj_path = (*i)["path"].to_string();
		const Redland::Node& val_node = (*i)["value"];
		target->set_port_value(obj_path, AtomRDF::node_to_atom(val_node));
	}
	world->rdf_world->mutex().unlock();


	/* **** Now query out objects **** */
	
	if (object_uri)
		query_str = Glib::ustring("SELECT DISTINCT ?class WHERE { <") + object_uri.get() + "> a ?class . }";
	else
		query_str = Glib::ustring("SELECT DISTINCT ?subject ?class WHERE { ?subject a ?class . }");

	query = Redland::Query(*world->rdf_world, query_str);
	results = Redland::Query::Results(query.run(*world->rdf_world, model, base_uri));
	
	const Redland::Node patch_class(*world->rdf_world, res, NS_INGEN "Patch");
	const Redland::Node node_class(*world->rdf_world, res, NS_INGEN "Node");
	const Redland::Node internal_class(*world->rdf_world, res, NS_INGEN "Internal");
	const Redland::Node ladspa_class(*world->rdf_world, res, NS_INGEN "LADSPAPlugin");
	const Redland::Node in_port_class(*world->rdf_world, res, NS_LV2 "InputPort");
	const Redland::Node out_port_class(*world->rdf_world, res, NS_LV2 "OutputPort");
	const Redland::Node lv2_class(*world->rdf_world, res, NS_LV2 "Plugin");
	
	string subject_str = ((object_uri && object_uri.get() != "") ? object_uri.get() : base_uri);
	if (subject_str[0] == '/')
		subject_str = subject_str.substr(1);
	if (subject_str == "")
		subject_str = base_uri;
	
	const Redland::Node subject_uri(*world->rdf_world, res, subject_str);

	bool ret = false;
	std::string path_str;

	for (Redland::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		const Redland::Node& subject = (object_uri ? subject_uri : (*i)["subject"]);
		const Redland::Node& rdf_class = (*i)["class"];
		//cout << "SUBJECT: " << subject.to_c_string() << endl;
		if (!object_uri) {
			path_str = uri_relative_to_base(base_uri, subject.to_c_string());
			//cout << "BASE: " << base_uri.c_str() << endl;
			//cout << "PATH: " << path_str.c_str() << endl;
			if (path_str[0] != '/')
				path_str = string("/").append(path_str);
			if (!Path::is_valid(path_str)) {
				//cerr << "INVALID PATH: " << path_str << endl;
			} else if (Path(path_str).parent() != "/") {
				continue;
			}
		}
		
		const bool is_plugin =    (rdf_class == ladspa_class)
		                       || (rdf_class == lv2_class)
		                       || (rdf_class == internal_class);
		
		const bool is_object =    (rdf_class == patch_class)
		                       || (rdf_class == node_class)
		                       || (rdf_class == in_port_class)
		                       || (rdf_class == out_port_class);

		if (is_object) {
			Raul::Path path(path_str == "" ? "/" : path_str);
			
			if (path.parent() != "/")
				continue;

			if (rdf_class == patch_class) {
				ret = parse_patch(world, target, model, base_uri, engine_base,
						path_str, data);
				if (ret)
					target->set_variable(path, "ingen:document", Atom(base_uri.c_str()));
			} else if (rdf_class == node_class) {
				ret = parse_node(world, target, model,
						base_uri, Glib::ustring("<") + subject.to_c_string() + ">", path, data);
			} else if (rdf_class == in_port_class || rdf_class == out_port_class) {
				ret = parse_port(world, target, model,
						base_uri, Glib::ustring("<") + subject.to_c_string() + ">", path, data);
			} else if (rdf_class == ladspa_class || rdf_class == lv2_class || rdf_class == internal_class)
			if (ret == false) {
				cerr << "Failed to parse object " << object_uri << endl;
				return ret;
			}
		} else if (is_plugin) {
			if (path_str.length() > 0) {
				const string uri = path_str.substr(1);
				target->set_property(uri, "rdf:type", Atom(Atom::URI, rdf_class.to_c_string()));
			} else {
				cout << "ERROR: Plugin with no URI parsed, ignoring" << endl;
			}
		}

	}

	return ret;
}


bool
Parser::parse_patch(
		Ingen::Shared::World*                   world,
		Ingen::Shared::CommonInterface*         target,
		Redland::Model&                         model,
		const Glib::ustring&                    base_uri,
		Glib::ustring                           engine_base,
		const Glib::ustring&                    object_uri,
		boost::optional<GraphObject::Variables> data=boost::optional<GraphObject::Variables>())
{
	std::set<Path> created;
	uint32_t       patch_poly = 0;

	/* Use parameter overridden polyphony, if given */
	if (data) {
		GraphObject::Variables::iterator poly_param = data.get().find("ingen:polyphony");
		if (poly_param != data.get().end() && poly_param->second.type() == Atom::INT)
			patch_poly = poly_param->second.get_int32();
	}
	
	Glib::ustring subject = ((object_uri[0] == '<')
			? object_uri : Glib::ustring("<") + object_uri + ">");

	if (subject[0] == '<' && subject[1] == '/')
		subject = string("<").append(subject.substr(2));
	
	//cout << "**** LOADING PATCH URI " << object_uri << ", SUBJ " << subject
	//	<< ", ENG BASE " << engine_base << endl;

	/* Get polyphony from file (mandatory if not specified in parameters) */
	if (patch_poly == 0) {
		Redland::Query query(*world->rdf_world, Glib::ustring(
			"SELECT DISTINCT ?poly WHERE { ") + subject + " ingen:polyphony ?poly\n }");

		Redland::Query::Results results = query.run(*world->rdf_world, model, base_uri);

		if (results.size() == 0) {
			cerr << "[Parser] ERROR: No polyphony found!" << endl;
			cerr << "Query was:" << endl << query.string() << endl;
			return false;
		}

		const Redland::Node& poly_node = (*results.begin())["poly"];
		assert(poly_node.is_int());
		patch_poly = static_cast<uint32_t>(poly_node.to_int());
	}

	string symbol = uri_relative_to_base(base_uri, object_uri);
	symbol = symbol.substr(0, symbol.find("."));
	Path patch_path("/");
	if (engine_base == "")
		patch_path = "/";
	else if (engine_base[engine_base.length()-1] == '/')
		patch_path = Path(engine_base + symbol);
	else if (Path::is_valid(engine_base))
		patch_path = (Path)engine_base;
	else if (Path::is_valid(string("/").append(engine_base)))
		patch_path = (Path)(string("/").append(engine_base));
	else
		cerr << "WARNING: Illegal engine base path '" << engine_base << "', loading patch to root" << endl;	

	//if (patch_path != "/")
		target->new_patch(patch_path, patch_poly);
	

	/* Plugin nodes */
	Redland::Query query(*world->rdf_world, Glib::ustring(
		"SELECT DISTINCT ?name ?plugin ?varkey ?varval ?poly WHERE {\n") +
		subject + " ingen:node       ?node .\n"
		"?node      lv2:symbol       ?name ;\n"
		"           ingen:plugin     ?plugin ;\n"
		"           ingen:polyphonic ?poly .\n"
		"OPTIONAL { ?node     lv2var:variable ?variable .\n"
		"           ?variable rdf:predicate   ?varkey ;\n"
		"                     rdf:value       ?varval .\n"
		"         }"
		"}");

	Redland::Query::Results results = query.run(*world->rdf_world, model, base_uri);
	world->rdf_world->mutex().lock();
	for (Redland::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		const string node_name = (*i)["name"].to_string();
		const Path   node_path = patch_path.base() + node_name;

		if (created.find(node_path) == created.end()) {
			const string node_plugin     = (*i)["plugin"].to_string();
			bool         node_polyphonic = false;

			const Redland::Node& poly_node = (*i)["poly"];
			if (poly_node.is_bool() && poly_node.to_bool() == true)
				node_polyphonic = true;
		
			target->new_node(node_path, node_plugin);
			target->set_property(node_path, "ingen:polyphonic", node_polyphonic);
			created.insert(node_path);
		}

		const string key = world->rdf_world->prefixes().qualify((*i)["varkey"].to_string());
		const Redland::Node& val_node = (*i)["varval"];

		if (key != "")
			target->set_variable(node_path, key, AtomRDF::node_to_atom(val_node));
	}
	world->rdf_world->mutex().unlock();
		

	/* Load subpatches */
	query = Redland::Query(*world->rdf_world, Glib::ustring(
		"SELECT DISTINCT ?subpatch ?symbol WHERE {\n") +
		subject + " ingen:node   ?subpatch .\n"
		"?subpatch  a            ingen:Patch ;\n"
		"           lv2:symbol   ?symbol .\n"
		"}");

	results = query.run(*world->rdf_world, model, base_uri);
	for (Redland::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		const string symbol = (*i)["symbol"].to_string();
		const string subpatch  = (*i)["subpatch"].to_string();

		const Path subpatch_path = patch_path.base() + symbol;

		if (created.find(subpatch_path) == created.end()) {
			string subpatch_rel =  uri_relative_to_base(base_uri, subpatch);
			string sub_base = engine_base;
			if (sub_base[sub_base.length()-1] == '/')
				sub_base = sub_base.substr(sub_base.length()-1);
			sub_base.append("/").append(symbol);
			created.insert(subpatch_path);
			parse_patch(world, target, model, base_uri, subpatch_rel, sub_base);
		}
	}


	/* Set node port control values */
	query = Redland::Query(*world->rdf_world, Glib::ustring(
		"SELECT DISTINCT ?nodename ?portname ?portval WHERE {\n") +
		subject + " ingen:node   ?node .\n"
		"?node      lv2:symbol   ?nodename ;\n"
		"           lv2:port     ?port .\n"
		"?port      lv2:symbol   ?portname ;\n"
		"           ingen:value  ?portval .\n"
		"FILTER ( datatype(?portval) = xsd:decimal )\n"
		"}");

	results = query.run(*world->rdf_world, model, base_uri);
	for (Redland::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		const string node_name = (*i)["nodename"].to_string();
		const string port_name = (*i)["portname"].to_string();

		assert(Path::is_valid_name(node_name));
		assert(Path::is_valid_name(port_name));
		const Path port_path = patch_path.base() + node_name + "/" + port_name;

		target->set_port_value(port_path, AtomRDF::node_to_atom((*i)["portval"]));
	}


	/* Load this patch's ports */
	query = Redland::Query(*world->rdf_world, Glib::ustring(
		"SELECT DISTINCT ?port ?type ?name ?datatype ?varkey ?varval ?portval WHERE {\n") +
		subject + " lv2:port       ?port .\n"
		"?port      a              ?type ;\n"
		"           a              ?datatype ;\n"
		"           lv2:symbol     ?name .\n"
		" FILTER (?type != ?datatype && ((?type = lv2:InputPort) || (?type = lv2:OutputPort)))\n"
		"OPTIONAL { ?port ingen:value ?portval . \n"
		"           FILTER ( datatype(?portval) = xsd:decimal ) }\n"
		"OPTIONAL { ?port     lv2var:variable ?variable .\n"
		"           ?variable rdf:predicate   ?varkey ;\n"
		"                     rdf:value       ?varval .\n"
		"         }"
		"}");

	results = query.run(*world->rdf_world, model, base_uri);
	world->rdf_world->mutex().lock();
	for (Redland::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		const string name     = (*i)["name"].to_string();
		const string type     = world->rdf_world->qualify((*i)["type"].to_string());
		const string datatype = world->rdf_world->qualify((*i)["datatype"].to_string());

		assert(Path::is_valid_name(name));
		const Path port_path = patch_path.base() + name;
			
		if (created.find(port_path) == created.end()) {
			bool is_output = (type == "lv2:OutputPort"); // FIXME: check validity
			// FIXME: read index
			target->new_port(port_path, datatype, 0, is_output);
			created.insert(port_path);
		}

		const Redland::Node& val_node = (*i)["portval"];
		target->set_port_value(patch_path.base() + name, AtomRDF::node_to_atom(val_node));

		const string key = world->rdf_world->prefixes().qualify((*i)["varkey"].to_string());
		const Redland::Node& var_val_node = (*i)["varval"];

		if (key != "")
			target->set_variable(patch_path.base() + name, key, AtomRDF::node_to_atom(var_val_node));
	}
	world->rdf_world->mutex().unlock();

	created.clear();
	
	parse_connections(world, target, model, base_uri, subject, patch_path);
	parse_variables(world, target, model, base_uri, subject, patch_path, data);

	/* Enable */
	query = Redland::Query(*world->rdf_world, Glib::ustring(
		"SELECT DISTINCT ?enabled WHERE { ") + subject + " ingen:enabled ?enabled }");

	results = query.run(*world->rdf_world, model, base_uri);
	for (Redland::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		const Redland::Node& enabled_node = (*i)["enabled"];
		if (enabled_node.is_bool() && enabled_node) {
			target->set_property(patch_path, "ingen:enabled", (bool)true);
			break;
		} else {
			cerr << "WARNING: Unknown type for property ingen:enabled" << endl;
		}
	}

	return true;
}


bool
Parser::parse_node(
		Ingen::Shared::World*                   world,
		Ingen::Shared::CommonInterface*         target,
		Redland::Model&                         model,
		const Glib::ustring&                    base_uri,
		const Glib::ustring&                    subject,
		const Raul::Path&                       path,
		boost::optional<GraphObject::Variables> data=boost::optional<GraphObject::Variables>())
{
	/* Get plugin */
	Redland::Query query(*world->rdf_world, Glib::ustring(
			"SELECT DISTINCT ?plug WHERE { ") + subject + " ingen:plugin ?plug }");

	Redland::Query::Results results = query.run(*world->rdf_world, model, base_uri);

	if (results.size() == 0) {
		cerr << "[Parser] ERROR: Node missing mandatory ingen:plugin property" << endl;
		return false;
	}
	
	const Redland::Node& plugin_node = (*results.begin())["plug"];
	if (plugin_node.type() != Redland::Node::RESOURCE) {
		cerr << "[Parser] ERROR: node's ingen:plugin property is not a resource" << endl;
		return false;
	}

	target->new_node(path, world->rdf_world->expand_uri(plugin_node.to_c_string()));
	parse_variables(world, target, model, base_uri, subject, path, data);

	return true;
}


bool
Parser::parse_port(
		Ingen::Shared::World*                   world,
		Ingen::Shared::CommonInterface*         target,
		Redland::Model&                         model,
		const Glib::ustring&                    base_uri,
		const Glib::ustring&                    subject,
		const Raul::Path&                       path,
		boost::optional<GraphObject::Variables> data)
{
	Redland::Query query(*world->rdf_world, Glib::ustring(
		"SELECT DISTINCT ?type ?datatype ?value WHERE {\n") +
		subject + " a ?type ;\n"
		"           a ?datatype .\n"
		" FILTER (?type != ?datatype && ((?type = lv2:InputPort) || (?type = lv2:OutputPort)))\n"
		"OPTIONAL { " + subject + " ingen:value ?value . }\n"
		"}");
	
	Redland::Query::Results results = query.run(*world->rdf_world, model, base_uri);
	world->rdf_world->mutex().lock();

	for (Redland::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		const string type     = world->rdf_world->qualify((*i)["type"].to_string());
		const string datatype = world->rdf_world->qualify((*i)["datatype"].to_string());

		bool is_output = (type == "lv2:OutputPort");
		// FIXME: read index
		target->new_port(path, datatype, 0, is_output);
		
		const Redland::Node& val_node = (*i)["value"];
		if (val_node.to_string() != "")
			target->set_port_value(path, AtomRDF::node_to_atom(val_node));
	}
	world->rdf_world->mutex().unlock();
	
	return parse_variables(world, target, model, base_uri, subject, path, data);
}


bool
Parser::parse_connections(
		Ingen::Shared::World*                   world,
		Ingen::Shared::CommonInterface*         target,
		Redland::Model&                         model,
		const Glib::ustring&                    base_uri,
		const Glib::ustring&                    subject,
		const Raul::Path&                       parent)
{
	Redland::Query query(*world->rdf_world, Glib::ustring(
		"SELECT DISTINCT ?src ?dst WHERE {\n")
		/*+ subject*/ + /*"?foo ingen:connection  ?connection .\n"*/
		"?connection  ingen:source      ?src ;\n"
		"             ingen:destination ?dst .\n"
		"}");

	Redland::Query::Results results = query.run(*world->rdf_world, model, base_uri);
	for (Redland::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		string src_path = parent.base() + uri_relative_to_base(base_uri, (*i)["src"].to_string());
		if (!Path::is_valid(src_path)) {
			cerr << "ERROR: Invalid path in connection: " << src_path << endl;
			continue;
		}
		
		string dst_path = parent.base() + uri_relative_to_base(base_uri, (*i)["dst"].to_string());
		if (!Path::is_valid(dst_path)) {
			cerr << "ERROR: Invalid path in connection: " << dst_path << endl;
			continue;
		}

		target->connect(src_path, dst_path);
	}

	return true;
}


bool
Parser::parse_variables(
		Ingen::Shared::World*                   world,
		Ingen::Shared::CommonInterface*         target,
		Redland::Model&                         model,
		const Glib::ustring&                    base_uri,
		const Glib::ustring&                    subject,
		const Raul::Path&                       path,
		boost::optional<GraphObject::Variables> data=boost::optional<GraphObject::Variables>())
{
	Redland::Query query(*world->rdf_world, Glib::ustring(
		"SELECT DISTINCT ?varkey ?varval WHERE {\n") +
		subject + " lv2var:variable ?variable .\n"
		"?variable  rdf:predicate   ?varkey ;\n"
		"           rdf:value       ?varval .\n"
		"}");

	Redland::Query::Results results = query.run(*world->rdf_world, model, base_uri);
	for (Redland::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		const string key = world->rdf_world->prefixes().qualify(string((*i)["varkey"]));
		const Redland::Node& val_node = (*i)["varval"];
		if (key != "")
			target->set_variable(path, key, AtomRDF::node_to_atom(val_node));
	}
	
	query = Redland::Query(*world->rdf_world, Glib::ustring(
		"SELECT DISTINCT ?key ?val WHERE {\n") +
		subject + " ingen:property ?property .\n"
		"?property  rdf:predicate  ?key ;\n"
		"           rdf:value      ?val .\n"
		"}");

	results = query.run(*world->rdf_world, model, base_uri);
	for (Redland::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		const string key = world->rdf_world->prefixes().qualify(string((*i)["key"]));
		const Redland::Node& val_node = (*i)["val"];
		if (key != "")
			target->set_property(path, key, AtomRDF::node_to_atom(val_node));
	}

	// Set passed variables last to override any loaded values
	if (data)
		for (GraphObject::Variables::const_iterator i = data.get().begin(); i != data.get().end(); ++i)
			target->set_variable(path, i->first, i->second);

	return true;
}


} // namespace Serialisation
} // namespace Ingen

