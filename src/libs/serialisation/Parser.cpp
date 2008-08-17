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
#include <redlandmm/Model.hpp>
#include <redlandmm/Node.hpp>
#include <redlandmm/Query.hpp>
#include <raul/TableImpl.hpp>
#include <raul/Atom.hpp>
#include <raul/AtomRDF.hpp>
#include "interface/EngineInterface.hpp"
#include "Parser.hpp"

using namespace std;
using namespace Raul;
using namespace Ingen::Shared;

namespace Ingen {
namespace Serialisation {

#define NS_INGEN "http://drobilla.net/ns/ingen#"


Glib::ustring
Parser::uri_relative_to_base(Glib::ustring base, const Glib::ustring uri)
{
	base = base.substr(0, base.find_last_of("/")+1);
	Glib::ustring ret;
	if (uri.length() > base.length() && uri.substr(0, base.length()) == base)
		ret = uri.substr(base.length());
	else
		ret = uri;
	return ret;
}


/** Parse a patch from RDF into a CommonInterface (engine or client).
 *
 * @param document_uri URI of file to load objects from.
 * @param parent Path of parent under which to load objects.
 * @return whether or not load was successful.
 */
bool
Parser::parse_document(
		Ingen::Shared::World*                   world,
		Ingen::Shared::CommonInterface*         target,
		const Glib::ustring&                    document_uri,
		Glib::ustring                           object_uri,
		boost::optional<Raul::Path>             parent,
		boost::optional<Raul::Symbol>           symbol,
		boost::optional<GraphObject::Variables> data)
{
	Redland::Model model(*world->rdf_world, document_uri, document_uri);

	if (object_uri == document_uri || object_uri == "")
		cout << "[Parser] Parsing document " << object_uri << endl;
	else
		cout << "[Parser] Parsing " << object_uri << " from " << document_uri << endl;

	return parse(world, target, model, document_uri, object_uri, parent, symbol, data);;
}


bool
Parser::parse_string(
		Ingen::Shared::World*                   world,
		Ingen::Shared::CommonInterface*         target,
		const Glib::ustring&                    str,
		const Glib::ustring&                    base_uri,
		boost::optional<Glib::ustring>          object_uri,
		boost::optional<Raul::Path>             parent,
		boost::optional<Raul::Symbol>           symbol,
		boost::optional<GraphObject::Variables> data)
{
	Redland::Model model(*world->rdf_world, str.c_str(), str.length(), base_uri);
	
	if (object_uri)
		cout << "[Parser] Parsing " << object_uri.get() << " (base " << base_uri << ")" <<  endl;
	else
		cout << "[Parser] Parsing all objects found in string (base " << base_uri << ")" << endl;
	
	return parse(world, target, model, base_uri, object_uri, parent, symbol, data);;
}


bool
Parser::parse(
		Ingen::Shared::World*                   world,
		Ingen::Shared::CommonInterface*         target,
		Redland::Model&                         model,
		Glib::ustring                           base_uri,
		boost::optional<Glib::ustring>          object_uri,
		boost::optional<Raul::Path>             parent,
		boost::optional<Raul::Symbol>           symbol,
		boost::optional<GraphObject::Variables> data)
{
	//if (object_uri)
	//	object_uri = uri_relative_to_base(base_uri, object_uri.get());

	Glib::ustring query_str;
	if (object_uri)
		query_str = Glib::ustring("SELECT DISTINCT ?class WHERE { <") + object_uri.get() + "> a ?class . }";
	else
		query_str = Glib::ustring("SELECT DISTINCT ?subject ?class WHERE { ?subject a ?class . }");

	Redland::Query query(*world->rdf_world, query_str);
	Redland::Query::Results results = query.run(*world->rdf_world, model, base_uri);
	
	const Redland::Node patch_class(*world->rdf_world, Redland::Node::RESOURCE, NS_INGEN "Patch");
	const Redland::Node node_class(*world->rdf_world, Redland::Node::RESOURCE, NS_INGEN "Node");
	const Redland::Node port_class(*world->rdf_world, Redland::Node::RESOURCE, NS_INGEN "Port");
	
	const Redland::Node subject_uri(*world->rdf_world, Redland::Node::RESOURCE,
			(object_uri ? object_uri.get() : "http://example.org"));

	bool ret = false;

	for (Redland::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		const Redland::Node subject = (object_uri ? subject_uri : (*i)["subject"]);
		const Redland::Node rdf_class = (*i)["class"];
		if (rdf_class == patch_class || rdf_class == node_class || rdf_class == port_class) {
			Path path = parse_path(world, model, base_uri, subject.to_c_string(), parent, symbol);
			if (rdf_class == patch_class) {
				ret = parse_patch(world, target, model, base_uri, subject.to_c_string(), path, data);
				if (ret)
					target->set_variable(path, "ingen:document", Atom(base_uri.c_str()));
			} else if (rdf_class == node_class) {
				ret = parse_node(world, target, model,
						base_uri, Glib::ustring("<") + subject.to_c_string() + ">", path, data);
			} else if (rdf_class == port_class) {
				cout << "*** TODO: PARSE PORT" << endl;
			}
			if (ret == false) {
				cerr << "Failed to parse object " << object_uri << endl;
				return ret;
			}
		}

	}

	return ret;
}


Path
Parser::parse_path(Ingen::Shared::World*          world,
                   Redland::Model&                model,
                   const Glib::ustring&           base_uri,
                   const Glib::ustring&           object_uri,
                   boost::optional<Raul::Path>&   parent,
                   boost::optional<Raul::Symbol>& symbol)
{
	string subject = string("</") + uri_relative_to_base(base_uri, object_uri) + ">";

	Redland::Query query(*world->rdf_world, Glib::ustring(
		"SELECT DISTINCT ?sym WHERE { ") + subject + " ingen:symbol ?sym }");
	
	Redland::Query::Results results = query.run(*world->rdf_world, model, base_uri);
	if (results.size() > 0) {
		symbol = (*results.begin())["sym"].to_string();
	} else {
		const string sym = object_uri.substr(base_uri.find_last_of("/")+1);
		symbol = Symbol::symbolify(sym.substr(0, sym.find(".")));
	}
	
	Path ret;
	if (base_uri == object_uri)
		ret = (parent ? parent.get().base() : Path("/"));
	else
		ret = (parent ? parent.get().base() : Path("/")) + symbol.get();
	cout << "Parsing " << object_uri << " (base " << base_uri << ") to " << ret << endl;
	return ret;
}


bool
Parser::parse_patch(
		Ingen::Shared::World*                   world,
		Ingen::Shared::CommonInterface*         target,
		Redland::Model&                         model,
		const Glib::ustring&                    base_uri,
		const Glib::ustring&                    object_uri,
		Raul::Path                              patch_path,
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

	const Glib::ustring subject = ((object_uri[0] == '<')
			? object_uri : Glib::ustring("<") + object_uri + ">");
	
	/* Get polyphony from file (mandatory if not specified in parameters) */
	if (patch_poly == 0) {
		Redland::Query query(*world->rdf_world, Glib::ustring(
			"SELECT DISTINCT ?poly WHERE { ") + subject + " ingen:polyphony ?poly\n }");

		Redland::Query::Results results = query.run(*world->rdf_world, model, base_uri);

		if (results.size() == 0) {
			cerr << "[Parser] ERROR: No polyphony found!" << endl;
			return false;
		}

		const Redland::Node& poly_node = (*results.begin())["poly"];
		assert(poly_node.is_int());
		patch_poly = static_cast<uint32_t>(poly_node.to_int());
	}

	if (patch_path != "/")
		target->new_patch(patch_path, patch_poly);
	
	/* Plugin nodes */
	Redland::Query query(*world->rdf_world, Glib::ustring(
		"SELECT DISTINCT ?name ?plugin ?varkey ?varval ?poly WHERE {\n") +
		subject + " ingen:node       ?node .\n"
		"?node      ingen:symbol     ?name ;\n"
		"           ingen:plugin     ?plugin ;\n"
		"           ingen:polyphonic ?poly .\n"
		"OPTIONAL { ?node     ingen:variable ?variable .\n"
		"           ?variable ingen:key      ?varkey ;\n"
		"                     ingen:value    ?varval .\n"
		"         }"
		"}");

	Redland::Query::Results results = query.run(*world->rdf_world, model, base_uri);
	world->rdf_world->mutex().lock();
	for (Redland::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		const string node_name = (*i)["name"].to_string();
		const Path   node_path = patch_path.base() + node_name;

		if (created.find(node_path) == created.end()) {
			const string node_plugin     = world->rdf_world->qualify((*i)["plugin"].to_string());
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
		"SELECT DISTINCT ?patch ?symbol WHERE {\n") +
		subject + " ingen:node   ?patch .\n"
		"?patch     a            ingen:Patch ;\n"
		"           ingen:symbol ?symbol .\n"
		"}");

	results = query.run(*world->rdf_world, model, base_uri);
	for (Redland::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		const string symbol = (*i)["symbol"].to_string();
		const string patch  = (*i)["patch"].to_string();

		const Path subpatch_path = patch_path.base() + symbol;

		if (created.find(subpatch_path) == created.end()) {
			created.insert(subpatch_path);
			parse_patch(world, target, model, base_uri, Glib::ustring(object_uri + subpatch_path), subpatch_path);
		}
	}


	/* Set node port control values */
	query = Redland::Query(*world->rdf_world, Glib::ustring(
		"SELECT DISTINCT ?nodename ?portname ?portval WHERE {\n") +
		subject + " ingen:node   ?node .\n"
		"?node      ingen:symbol ?nodename ;\n"
		"           ingen:port   ?port .\n"
		"?port      ingen:symbol ?portname ;\n"
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
		subject + " ingen:port     ?port .\n"
		"?port      a              ?type ;\n"
		"           a              ?datatype ;\n"
		"           ingen:symbol   ?name .\n"
		" FILTER (?type != ?datatype && ((?type = ingen:InputPort) || (?type = ingen:OutputPort)))\n"
		"OPTIONAL { ?port ingen:value ?portval . \n"
		"           FILTER ( datatype(?portval) = xsd:decimal ) }\n"
		"OPTIONAL { ?port     ingen:variable ?variable .\n"
		"           ?variable ingen:key      ?varkey ;\n"
		"                     ingen:value    ?varval .\n"
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
			bool is_output = (type == "ingen:OutputPort"); // FIXME: check validity
			// FIXME: read index
			target->new_port(port_path, 0, datatype, is_output);
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
	
	/* Connections */
	query = Redland::Query(*world->rdf_world, Glib::ustring(
		"SELECT DISTINCT ?src ?dst WHERE {\n") +
		subject +  " ingen:connection ?connection .\n"
		"?connection ingen:source ?src ;\n"
		"            ingen:destination ?dst .\n"
		"}");

	results = query.run(*world->rdf_world, model, base_uri);
	for (Redland::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		string src_path = patch_path.base() + uri_relative_to_base(base_uri, (*i)["src"].to_string());
		if (!Path::is_valid(src_path)) {
			cerr << "ERROR: Invalid path in connection: " << src_path << endl;
			continue;
		}
		
		string dst_path = patch_path.base() + uri_relative_to_base(base_uri, (*i)["dst"].to_string());
		if (!Path::is_valid(dst_path)) {
			cerr << "ERROR: Invalid path in connection: " << dst_path << endl;
			continue;
		}

		target->connect(src_path, dst_path);
	}

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
		Raul::Path                              path,
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

	target->new_node(path, plugin_node.to_c_string());
	parse_variables(world, target, model, base_uri, subject, path, data);

	return true;
}



bool
Parser::parse_variables(
		Ingen::Shared::World*                   world,
		Ingen::Shared::CommonInterface*         target,
		Redland::Model&                         model,
		const Glib::ustring&                    base_uri,
		const Glib::ustring&                    subject,
		Raul::Path                              path,
		boost::optional<GraphObject::Variables> data=boost::optional<GraphObject::Variables>())
{
	/* Parse variables */
	Redland::Query query = Redland::Query(*world->rdf_world, Glib::ustring(
		"SELECT DISTINCT ?varkey ?varval WHERE {\n") +
		subject + " ingen:variable ?variable .\n"
		"?variable  ingen:key      ?varkey ;\n"
		"           ingen:value    ?varval .\n"
		"}");

	Redland::Query::Results results = query.run(*world->rdf_world, model, base_uri);
	for (Redland::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		const string key = world->rdf_world->prefixes().qualify(string((*i)["varkey"]));
		const Redland::Node& val_node = (*i)["varval"];
		if (key != "")
			target->set_variable(path, key, AtomRDF::node_to_atom(val_node));
	}

	// Set passed variables last to override any loaded values
	if (data)
		for (GraphObject::Variables::const_iterator i = data.get().begin(); i != data.get().end(); ++i)
			target->set_variable(path, i->first, i->second);

	return true;
}


} // namespace Serialisation
} // namespace Ingen

