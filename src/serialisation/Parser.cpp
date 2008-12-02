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
Parser::relative_uri(Glib::ustring base, const Glib::ustring uri, bool leading_slash)
{
	base = base.substr(0, base.find_last_of("/")+1);
	Glib::ustring ret;
	if (uri.length() >= base.length() && uri.substr(0, base.length()) == base)
		ret = uri.substr(base.length());
	else
		ret = uri;

	if (leading_slash && (ret == "" || ret[0] != '/'))
		ret = "/" + ret;
	else if (!leading_slash && ret != "" && ret[0] == '/')
		ret = ret.substr(1);

	return ret;
}


static void
normalise_uri(Glib::ustring& uri)
{
	size_t dotslash = string::npos;
	while ((dotslash = uri.find("./")) != string::npos)
		uri = uri.substr(0, dotslash) + uri.substr(dotslash + 2);
}


/** Parse a patch from RDF into a CommonInterface (engine or client).
 * @return whether or not load was successful.
 */
bool
Parser::parse_document(
		Ingen::Shared::World*                   world,
		Ingen::Shared::CommonInterface*         target,
		Glib::ustring                           document_uri,
		boost::optional<Raul::Path>             data_path,
		boost::optional<Raul::Path>             parent,
		boost::optional<Raul::Symbol>           symbol,
		boost::optional<GraphObject::Variables> data)
{
	normalise_uri(document_uri);
	
	Redland::Model model(*world->rdf_world, document_uri, document_uri);

	cout << "[Parser] Parsing document " << document_uri << endl;
	if (data_path)
		cout << "[Parser] Document path: " << *data_path << endl;
	if (parent)
		cout << "[Parser] Parent: " << *parent << endl;
	if (symbol)
		cout << "[Parser] Symbol: " << *symbol << endl;

	boost::optional<Path> parsed_path
		= parse(world, target, model, document_uri, data_path, parent, symbol, data);
	
	if (parsed_path) {
		target->set_variable(*parsed_path, "ingen:document", Atom(document_uri.c_str()));
	} else {
		cerr << "WARNING: document URI lost" << endl;
	}
		
	return parsed_path;
}


bool
Parser::parse_string(
		Ingen::Shared::World*                   world,
		Ingen::Shared::CommonInterface*         target,
		const Glib::ustring&                    str,
		const Glib::ustring&                    base_uri,
		boost::optional<Raul::Path>             data_path,
		boost::optional<Raul::Path>             parent,
		boost::optional<Raul::Symbol>           symbol,
		boost::optional<GraphObject::Variables> data)
{
	Redland::Model model(*world->rdf_world, str.c_str(), str.length(), base_uri);
	
	cout << "Parsing " << (data_path ? (string)*data_path : "*")
		<< " from string (base " << base_uri << ")" <<  endl;
	
	cout << "Parent: " << (parent ? *parent : "/no/bo/dy") << endl;;
	bool ret = parse(world, target, model, base_uri, data_path, parent, symbol, data);
	const Glib::ustring subject = Glib::ustring("<") + base_uri + Glib::ustring(">");
	parse_connections(world, target, model, subject, parent ? *parent : "/");

	return ret;
}


bool
Parser::parse_update(
		Ingen::Shared::World*                   world,
		Shared::CommonInterface*                target,
		const Glib::ustring&                    str,
		const Glib::ustring&                    base_uri,
		boost::optional<Raul::Path>             data_path,
		boost::optional<Raul::Path>             parent,
		boost::optional<Raul::Symbol>           symbol,
		boost::optional<GraphObject::Variables> data)
{
	Redland::Model model(*world->rdf_world, str.c_str(), str.length(), base_uri);

	// Delete anything explicitly declared to not exist
	Glib::ustring query_str = Glib::ustring("SELECT DISTINCT ?o WHERE { ?o a owl:Nothing }");
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

	for (Redland::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		world->rdf_world->mutex().lock();
		const string obj_path = (*i)["path"].to_string();
		const string key = world->rdf_world->prefixes().qualify((*i)["varkey"].to_string());
		const Redland::Node& val_node = (*i)["varval"];
		const Atom a(AtomRDF::node_to_atom(val_node));
		world->rdf_world->mutex().unlock();
		if (key != "")
			target->set_variable(obj_path, key, a);
	}


	// Connections
	parse_connections(world, target, model, base_uri, "/");
	
	// Port values
	query = Redland::Query(*world->rdf_world,
		"SELECT DISTINCT ?path ?value WHERE {\n"
		"?path ingen:value ?value .\n"
		"}");
	
	results = Redland::Query::Results(query.run(*world->rdf_world, model, base_uri));

	for (Redland::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		world->rdf_world->mutex().lock();
		const string obj_path = (*i)["path"].to_string();
		const Redland::Node& val_node = (*i)["value"];
		const Atom a(AtomRDF::node_to_atom(val_node));
		world->rdf_world->mutex().unlock();
		target->set_port_value(obj_path, a);
	}

	return parse(world, target, model, base_uri, data_path, parent, symbol, data);
}


boost::optional<Path>
Parser::parse(
		Ingen::Shared::World*                   world,
		Ingen::Shared::CommonInterface*         target,
		Redland::Model&                         model,
		Glib::ustring                           document_uri,
		boost::optional<Raul::Path>             data_path,
		boost::optional<Raul::Path>             parent,
		boost::optional<Raul::Symbol>           symbol,
		boost::optional<GraphObject::Variables> data)
{
	const Redland::Node::Type res = Redland::Node::RESOURCE;
	
	const Glib::ustring query_str = data_path
		? Glib::ustring("SELECT DISTINCT ?t WHERE { <") + data_path->substr(1) + "> a ?t . }"
		: Glib::ustring("SELECT DISTINCT ?s ?t WHERE { ?s a ?t . }");

	Redland::Query query(*world->rdf_world, query_str);
	Redland::Query::Results results(query.run(*world->rdf_world, model, document_uri));
	
	const Redland::Node patch_class    (*world->rdf_world, res, NS_INGEN "Patch");
	const Redland::Node node_class     (*world->rdf_world, res, NS_INGEN "Node");
	const Redland::Node internal_class (*world->rdf_world, res, NS_INGEN "Internal");
	const Redland::Node ladspa_class   (*world->rdf_world, res, NS_INGEN "LADSPAPlugin");
	const Redland::Node in_port_class  (*world->rdf_world, res, NS_LV2   "InputPort");
	const Redland::Node out_port_class (*world->rdf_world, res, NS_LV2   "OutputPort");
	const Redland::Node lv2_class      (*world->rdf_world, res, NS_LV2   "Plugin");
	
	const Redland::Node subject_node = (data_path && *data_path != "/")
		? Redland::Node(*world->rdf_world, res, data_path->substr(1))
		: model.base_uri();

	std::string           path_str;
	boost::optional<Path> ret;
	boost::optional<Path> root_path;

	for (Redland::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		const Redland::Node& subject = (data_path ? subject_node : (*i)["s"]);
		const Redland::Node& rdf_class = (*i)["t"];

		//cout << "SUBJECT: " << subject.to_c_string() << endl;
		//cout << "CLASS:   " << rdf_class.to_c_string() << endl;

		if (!data_path)
			path_str = relative_uri(document_uri, subject.to_c_string(), true);
		else if (path_str == "" || path_str[0] != '/')
			path_str = "/" + path_str;
			
		if (!Path::is_valid(path_str)) {
			cerr << "WARNING: Invalid path '" << path_str << "', object skipped" << endl;
			continue;
		}
		
		const bool is_plugin =    (rdf_class == ladspa_class)
		                       || (rdf_class == lv2_class)
		                       || (rdf_class == internal_class);
		
		const bool is_object =    (rdf_class == patch_class)
		                       || (rdf_class == node_class)
		                       || (rdf_class == in_port_class)
		                       || (rdf_class == out_port_class);
		
		const Glib::ustring subject_uri_tok = Glib::ustring("<").append(subject).append(">");

		if (is_object) {

			string path;
			if (parent && symbol) {
				path = parent->base() + *symbol;
			} else {
				path = (parent ? parent->base() : "/") + path_str.substr(1);
			}

			if (!Path::is_valid(path)) {
				cerr << "WARNING: Invalid path '" << path << "' transformed to /" << endl;
				path = "/";
			}
			
			if (rdf_class == patch_class) {
				ret = parse_patch(world, target, model, subject, parent, symbol, data);
			} else if (rdf_class == node_class) {
				ret = parse_node(world, target, model, subject, path, data);
			} else if (rdf_class == in_port_class || rdf_class == out_port_class) {
				ret = parse_port(world, target, model, subject, path, data);
			}

			if (!ret) {
				cerr << "Failed to parse object " << path << endl;
				return boost::optional<Path>();
			}
				
			if (data_path && subject.to_string() == *data_path)
				root_path = ret;

		} else if (is_plugin) {
			if (path_str.length() > 0)
				target->set_property(path_str, "rdf:type", Atom(Atom::URI, rdf_class.to_c_string()));
		}

	}
	
	return root_path;
}


boost::optional<Path>
Parser::parse_patch(
		Ingen::Shared::World*                   world,
		Ingen::Shared::CommonInterface*         target,
		Redland::Model&                         model,
		const Redland::Node&                    subject_node,
		boost::optional<Raul::Path>             parent,
		boost::optional<Raul::Symbol>           a_symbol,
		boost::optional<GraphObject::Variables> data)
{
	std::set<Path> created;
	uint32_t       patch_poly = 0;

	/* Use parameter overridden polyphony, if given */
	if (data) {
		GraphObject::Variables::iterator poly_param = data.get().find("ingen:polyphony");
		if (poly_param != data.get().end() && poly_param->second.type() == Atom::INT)
			patch_poly = poly_param->second.get_int32();
	}
	
	const Glib::ustring subject = subject_node.to_turtle_token();

	//cout << "Parse patch " << subject << endl;

	/* Get polyphony from file (mandatory if not specified in parameters) */
	if (patch_poly == 0) {
		Redland::Query query(*world->rdf_world, Glib::ustring(
			"SELECT DISTINCT ?poly WHERE { ") + subject + " ingen:polyphony ?poly }");

		Redland::Query::Results results = query.run(*world->rdf_world, model);

		if (results.size() == 0) {
			cerr << "[Parser] ERROR: No polyphony found!" << endl;
			cerr << "Query was:" << endl << query.string() << endl;
			return boost::optional<Path>();
		}

		const Redland::Node& poly_node = (*results.begin())["poly"];
		assert(poly_node.is_int());
		patch_poly = static_cast<uint32_t>(poly_node.to_int());
	}
	
	const Glib::ustring base_uri = model.base_uri().to_string();

	string symbol;
	if (a_symbol) {
		symbol = *a_symbol;
	} else { // Guess symbol from base URI (filename) if we need to
		symbol = base_uri.substr(base_uri.find_last_of("/") + 1);
		symbol = symbol.substr(0, symbol.find("."));
		if (symbol != "")
			symbol = Path::nameify(symbol);
	}
		
	const Path patch_path(parent ? parent->base() + symbol : "/");
	target->new_patch(patch_path, patch_poly);
	

	/* Plugin nodes */
	Redland::Query query(*world->rdf_world, Glib::ustring(
		"SELECT DISTINCT ?name ?plugin ?varkey ?varval ?poly WHERE {\n") +
		subject + " ingen:node       ?node .\n"
		"?node      lv2:symbol       ?name ;\n"
		"           rdf:instanceOf   ?plugin ;\n"
		"           ingen:polyphonic ?poly .\n"
		"OPTIONAL { ?node     lv2var:variable ?variable .\n"
		"           ?variable rdf:predicate   ?varkey ;\n"
		"                     rdf:value       ?varval .\n"
		"         }"
		"}");

	Redland::Query::Results results = query.run(*world->rdf_world, model, base_uri);

	for (Redland::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		world->rdf_world->mutex().lock();
		const string node_name = (*i)["name"].to_string();
		const Path   node_path = patch_path.base() + node_name;
		
		if (created.find(node_path) == created.end()) {
			const string node_plugin     = (*i)["plugin"].to_string();
			bool         node_polyphonic = false;

			const Redland::Node& poly_node = (*i)["poly"];
			if (poly_node.is_bool() && poly_node.to_bool() == true)
				node_polyphonic = true;
			
			world->rdf_world->mutex().unlock();
		
			target->new_node(node_path, node_plugin);
			target->set_property(node_path, "ingen:polyphonic", node_polyphonic);
			created.insert(node_path);
			
			world->rdf_world->mutex().lock();
		}

		const string key = world->rdf_world->prefixes().qualify((*i)["varkey"].to_string());
		const Redland::Node& val_node = (*i)["varval"];
		
		world->rdf_world->mutex().unlock();

		if (key != "")
			target->set_variable(node_path, key, AtomRDF::node_to_atom(val_node));
	}


	/* Load subpatches */
	query = Redland::Query(*world->rdf_world, Glib::ustring(
		"SELECT DISTINCT ?subpatch ?symbol WHERE {\n") +
		subject + " ingen:node   ?subpatch .\n"
		"?subpatch  a            ingen:Patch ;\n"
		"           lv2:symbol   ?symbol .\n"
		"}");

	results = query.run(*world->rdf_world, model, base_uri);
	for (Redland::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		world->rdf_world->mutex().lock();
		const string symbol   = (*i)["symbol"].to_string();
		const string subpatch = (*i)["subpatch"].to_string();
		world->rdf_world->mutex().unlock();

		const Path subpatch_path = patch_path.base() + symbol;

		if (created.find(subpatch_path) == created.end()) {
			/*const string subpatch_rel = relative_uri(base_uri, subpatch);
			string sub_base = engine_base;
			if (sub_base[sub_base.length()-1] == '/')
				sub_base = sub_base.substr(sub_base.length()-1);
			sub_base.append("/").append(symbol);*/
			created.insert(subpatch_path);
			parse_patch(world, target, model, (*i)["subpatch"], patch_path, Symbol(symbol));
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
		world->rdf_world->mutex().lock();
		const string node_name = (*i)["nodename"].to_string();
		const string port_name = (*i)["portname"].to_string();
		world->rdf_world->mutex().unlock();

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

	for (Redland::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		world->rdf_world->mutex().lock();
		const string name     = (*i)["name"].to_string();
		const string type     = world->rdf_world->qualify((*i)["type"].to_string());
		const string datatype = world->rdf_world->qualify((*i)["datatype"].to_string());
		world->rdf_world->mutex().unlock();

		assert(Path::is_valid_name(name));
		const Path port_path = patch_path.base() + name;
			
		if (created.find(port_path) == created.end()) {
			// TODO: read index for plugin wrapper
			bool is_output = (type == "lv2:OutputPort");
			target->new_port(port_path, datatype, 0, is_output);
			created.insert(port_path);
		}

		world->rdf_world->mutex().lock();
		const Redland::Node& val_node = (*i)["portval"];
		world->rdf_world->mutex().unlock();

		target->set_port_value(patch_path.base() + name, AtomRDF::node_to_atom(val_node));

		world->rdf_world->mutex().lock();
		const string key = world->rdf_world->prefixes().qualify((*i)["varkey"].to_string());
		const Redland::Node& var_val_node = (*i)["varval"];
		world->rdf_world->mutex().unlock();

		if (key != "")
			target->set_variable(patch_path.base() + name, key, AtomRDF::node_to_atom(var_val_node));
	}


	created.clear();
	
	parse_connections(world, target, model, subject, patch_path);
	parse_variables(world, target, model, subject_node, patch_path, data);

	/* Enable */
	query = Redland::Query(*world->rdf_world, Glib::ustring(
		"SELECT DISTINCT ?enabled WHERE { ") + subject + " ingen:enabled ?enabled }");

	results = query.run(*world->rdf_world, model, base_uri);
	for (Redland::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		world->rdf_world->mutex().lock();
		const Redland::Node& enabled_node = (*i)["enabled"];
		world->rdf_world->mutex().unlock();
		if (enabled_node.is_bool() && enabled_node) {
			target->set_property(patch_path, "ingen:enabled", (bool)true);
			break;
		} else {
			cerr << "WARNING: Unknown type for property ingen:enabled" << endl;
		}
	}

	return patch_path;
}


boost::optional<Path>
Parser::parse_node(
		Ingen::Shared::World*                   world,
		Ingen::Shared::CommonInterface*         target,
		Redland::Model&                         model,
		const Redland::Node&                    subject,
		const Raul::Path&                       path,
		boost::optional<GraphObject::Variables> data)
{
	/* Get plugin */
	Redland::Query query(*world->rdf_world, Glib::ustring(
			"SELECT DISTINCT ?plug WHERE { ") + subject.to_turtle_token()
			+ " rdf:instanceOf ?plug }");

	Redland::Query::Results results = query.run(*world->rdf_world, model);

	if (results.size() == 0) {
		cerr << "[Parser] ERROR: Node missing mandatory rdf:instanceOf property" << endl;
		return boost::optional<Path>();
	}
	
	const Redland::Node& plugin_node = (*results.begin())["plug"];
	if (plugin_node.type() != Redland::Node::RESOURCE) {
		cerr << "[Parser] ERROR: node's rdf:instanceOf property is not a resource" << endl;
		return boost::optional<Path>();
	}

	target->new_node(path, world->rdf_world->expand_uri(plugin_node.to_c_string()));
	parse_variables(world, target, model, subject, path, data);
	return path;
}


boost::optional<Path>
Parser::parse_port(
		Ingen::Shared::World*                   world,
		Ingen::Shared::CommonInterface*         target,
		Redland::Model&                         model,
		const Redland::Node&                    subject_node,
		const Raul::Path&                       path,
		boost::optional<GraphObject::Variables> data)
{
	const Glib::ustring subject = subject_node.to_turtle_token();

	Redland::Query query(*world->rdf_world, Glib::ustring(
		"SELECT DISTINCT ?type ?datatype ?value WHERE {\n") +
		subject + " a ?type ;\n"
		"           a ?datatype .\n"
		" FILTER (?type != ?datatype && ((?type = lv2:InputPort) || (?type = lv2:OutputPort)))\n"
		"OPTIONAL { " + subject + " ingen:value ?value . }\n"
		"}");

	Redland::Query::Results results = query.run(*world->rdf_world, model);
	world->rdf_world->mutex().lock();

	for (Redland::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		const string type     = world->rdf_world->qualify((*i)["type"].to_string());
		const string datatype = world->rdf_world->qualify((*i)["datatype"].to_string());

		// TODO: read index for plugin wrapper
		bool is_output = (type == "lv2:OutputPort");
		target->new_port(path, datatype, 0, is_output);
		
		const Redland::Node& val_node = (*i)["value"];
		if (val_node.to_string() != "")
			target->set_port_value(path, AtomRDF::node_to_atom(val_node));
	}
	world->rdf_world->mutex().unlock();
	
	parse_variables(world, target, model, subject_node, path, data);
	return path;
}


bool
Parser::parse_connections(
		Ingen::Shared::World*           world,
		Ingen::Shared::CommonInterface* target,
		Redland::Model&                 model,
		const Glib::ustring&            subject,
		const Raul::Path&               parent)
{
	Redland::Query query(*world->rdf_world, Glib::ustring(
		"SELECT DISTINCT ?src ?dst WHERE {\n")
		+ subject + " ingen:connection  ?connection .\n"
		"?connection  ingen:source      ?src ;\n"
		"             ingen:destination ?dst .\n"
		"}");

	const Glib::ustring& base_uri = model.base_uri().to_string();
	const Glib::ustring& base = base_uri.substr(0, base_uri.find_last_of("/") + 1);

	Redland::Query::Results results = query.run(*world->rdf_world, model);
	for (Redland::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		const string src_path = parent.base() + relative_uri(base, (*i)["src"].to_string(), false);
		if (!Path::is_valid(src_path)) {
			cerr << "ERROR: Invalid path in connection: " << src_path << endl;
			continue;
		}
		
		const string dst_path = parent.base() + relative_uri(base, (*i)["dst"].to_string(), false);
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
		const Redland::Node&                    subject_node,
		const Raul::Path&                       path,
		boost::optional<GraphObject::Variables> data)
{
	const Glib::ustring& subject = subject_node.to_turtle_token();

	Redland::Query query(*world->rdf_world, Glib::ustring(
		"SELECT DISTINCT ?varkey ?varval WHERE {\n") +
		subject + " lv2var:variable ?variable .\n"
		"?variable  rdf:predicate   ?varkey ;\n"
		"           rdf:value       ?varval .\n"
		"}");

	Redland::Query::Results results = query.run(*world->rdf_world, model);
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

	results = query.run(*world->rdf_world, model);
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

