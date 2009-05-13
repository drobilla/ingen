/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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

static Glib::ustring
relative_uri(Glib::ustring base, const Glib::ustring uri, bool leading_slash)
{
	Glib::ustring ret;
	if (uri.substr(0, base.length()) == base) {
		ret = (leading_slash ? "/" : "") + uri.substr(base.length());
		while (ret[0] == '#' || ret[0] == '/')
			ret = ret.substr(1);
		if (leading_slash && ret[0] != '/')
			ret = string("/") + ret;
		return ret;
	}

	size_t last_slash = base.find_last_of("/");
	if (last_slash != string::npos)
		base = base.substr(0, last_slash + 1);

	size_t last_hash = base.find_last_of("#");
	if (last_hash != string::npos)
		base = base.substr(0, last_hash + 1);

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


#if 0
static Glib::ustring
chop_uri_scheme(const Glib::ustring in)
{
	Glib::ustring out = in;

	// Remove scheme
	size_t scheme_end = out.find(":");
	if (scheme_end != string::npos)
		out = out.substr(scheme_end + 1);

	// Chop leading double slashes
	while (out.length() > 1 && out[0] == '/' && out[1] == '/')
		out = out.substr(1);

	return out;
}


static Glib::ustring
uri_child(const Glib::ustring base, const Glib::ustring child, bool trailing_slash)
{
	Glib::ustring ret = (base[base.length()-1] == '/' || child[0] == '/')
		? base + child
		: base + '/' + child;

	if (trailing_slash && (ret == "" || ret[ret.length()-1] != '/'))
		ret = ret + "/";
	else if (!trailing_slash && ret != "" && ret[ret.length()-1] == '/')
		ret = ret.substr(0, ret.length()-1);

	return ret;
}


static Glib::ustring
dequote_uri(const Glib::ustring in)
{
	Glib::ustring out = in;
	if (out[0] == '<' && out[out.length()-1] == '>')
		out = out.substr(1, out.length()-2);
	return out;
}
#endif


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
		Ingen::Shared::World*                    world,
		Ingen::Shared::CommonInterface*          target,
		Glib::ustring                            document_uri,
		boost::optional<Raul::Path>              data_path,
		boost::optional<Raul::Path>              parent,
		boost::optional<Raul::Symbol>            symbol,
		boost::optional<GraphObject::Properties> data)
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
		Ingen::Shared::World*                    world,
		Ingen::Shared::CommonInterface*          target,
		const Glib::ustring&                     str,
		const Glib::ustring&                     base_uri,
		boost::optional<Raul::Path>              data_path,
		boost::optional<Raul::Path>              parent,
		boost::optional<Raul::Symbol>            symbol,
		boost::optional<GraphObject::Properties> data)
{
	Redland::Model model(*world->rdf_world, str.c_str(), str.length(), base_uri);

	cout << "Parsing " << (data_path ? data_path->str() : "*") << " from string";
	if (base_uri != "")
		cout << "(base " << base_uri << ")";
	cout << endl;

	bool ret = parse(world, target, model, base_uri, data_path, parent, symbol, data);
	const Glib::ustring subject = Glib::ustring("<") + base_uri + Glib::ustring(">");
	parse_connections(world, target, model, subject, parent ? *parent : "/");

	return ret;
}


bool
Parser::parse_update(
		Ingen::Shared::World*                    world,
		Shared::CommonInterface*                 target,
		const Glib::ustring&                     str,
		const Glib::ustring&                     base_uri,
		boost::optional<Raul::Path>              data_path,
		boost::optional<Raul::Path>              parent,
		boost::optional<Raul::Symbol>            symbol,
		boost::optional<GraphObject::Properties> data)
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
		Glib::Mutex::Lock lock(world->rdf_world->mutex());
		const string obj_path = (*i)["path"].to_string();
		const string key = world->rdf_world->prefixes().qualify((*i)["varkey"].to_string());
		const Redland::Node& val_node = (*i)["varval"];
		const Atom a(AtomRDF::node_to_atom(val_node));
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
		Glib::Mutex::Lock lock(world->rdf_world->mutex());
		const string obj_path = (*i)["path"].to_string();
		const Redland::Node& val_node = (*i)["value"];
		const Atom a(AtomRDF::node_to_atom(val_node));
		target->set_port_value(obj_path, a);
	}

	return parse(world, target, model, base_uri, data_path, parent, symbol, data);
}


boost::optional<Path>
Parser::parse(
		Ingen::Shared::World*                    world,
		Ingen::Shared::CommonInterface*          target,
		Redland::Model&                          model,
		Glib::ustring                            document_uri,
		boost::optional<Raul::Path>              data_path,
		boost::optional<Raul::Path>              parent,
		boost::optional<Raul::Symbol>            symbol,
		boost::optional<GraphObject::Properties> data)
{
	const Redland::Node::Type res = Redland::Node::RESOURCE;

	const Glib::ustring query_str = data_path
		? Glib::ustring("SELECT DISTINCT ?t WHERE { <") + data_path->chop_start("/") + "> a ?t . }"
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

	const Redland::Node subject_node = (data_path && !data_path->is_root())
		? Redland::Node(*world->rdf_world, res, data_path->chop_start("/"))
		: model.base_uri();

	std::string           path_str;
	boost::optional<Path> ret;
	boost::optional<Path> root_path;

	for (Redland::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		const Redland::Node& subject = (data_path ? subject_node : (*i)["s"]);
		const Redland::Node& rdf_class = (*i)["t"];

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

			string path = (parent && symbol)
					? parent->base() + *symbol
					: (parent ? parent->base() : "/") + path_str.substr(path_str.find("/")+1);

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

			if (data_path && subject.to_string() == data_path->str())
				root_path = ret;

		} else if (is_plugin) {
			if (URI::is_valid(path_str))
				target->set_property(path_str, "rdf:type", Atom(Atom::URI, rdf_class.to_c_string()));
		}

	}

	return root_path;
}


boost::optional<Path>
Parser::parse_patch(
		Ingen::Shared::World*                    world,
		Ingen::Shared::CommonInterface*          target,
		Redland::Model&                          model,
		const Redland::Node&                     subject_node,
		boost::optional<Raul::Path>              parent,
		boost::optional<Raul::Symbol>            a_symbol,
		boost::optional<GraphObject::Properties> data)
{
	uint32_t patch_poly = 0;

	typedef Redland::Query::Results Results;

	/* Use parameter overridden polyphony, if given */
	if (data) {
		GraphObject::Properties::iterator poly_param = data.get().find("ingen:polyphony");
		if (poly_param != data.get().end() && poly_param->second.type() == Atom::INT)
			patch_poly = poly_param->second.get_int32();
	}

	const Glib::ustring subject = subject_node.to_turtle_token();

	//cout << "**** Parse patch " << subject << endl;

	/* Load polyphony from file if necessary */
	if (patch_poly == 0) {
		Redland::Query query(*world->rdf_world, Glib::ustring(
			"SELECT DISTINCT ?poly WHERE { ") + subject + " ingen:polyphony ?poly }");

		Results results = query.run(*world->rdf_world, model);
		if (results.size() > 0) {
			const Redland::Node& poly_node = (*results.begin())["poly"];
			if (poly_node.is_int())
				patch_poly = poly_node.to_int();
			else
				cerr << "WARNING: Patch has non-integer polyphony, assuming 1" << endl;
		}
	}

	/* No polyphony value anywhere, 1 it is */
	if (patch_poly == 0)
		patch_poly = 1;

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

	string patch_path_str = relative_uri(base_uri, subject_node.to_string(), true);
	if (!Path::is_valid(patch_path_str)) {
		cerr << "ERROR: Patch has invalid path: " << patch_path_str << endl;
		return boost::optional<Raul::Path>();
	}


	/* Create patch */
	Path patch_path(patch_path_str);
	target->new_patch(patch_path, patch_poly);


	/* Find patches in document */
	Redland::Query query(*world->rdf_world, Glib::ustring(
		"SELECT DISTINCT ?patch WHERE {\n") +
			"?patch a ingen:Patch .\n"
		"}");
	set<string> patches;
	Results results = query.run(*world->rdf_world, model, base_uri);
	for (Results::iterator i = results.begin(); i != results.end(); ++i) {
		Glib::Mutex::Lock lock(world->rdf_world->mutex());
		patches.insert((*i)["patch"].to_string());
	}

	typedef multimap<std::string, Redland::Node> Properties;
	typedef map<string, Redland::Node>           Resources;
	typedef map<string, Properties>              Objects;
	typedef map<string, string>                  Types;


	/* Find nodes on this patch */
	query = Redland::Query(*world->rdf_world, Glib::ustring(
		"SELECT DISTINCT ?node ?type WHERE {\n") +
			subject + " ingen:node     ?node .\n"
			"?node      rdf:instanceOf ?type .\n"
		"}");
	Objects   patch_nodes;
	Objects   plugin_nodes;
	Resources resources;
	Types     types;
	results = query.run(*world->rdf_world, model, base_uri);
	for (Results::iterator i = results.begin(); i != results.end(); ++i) {
		Glib::Mutex::Lock lock(world->rdf_world->mutex());
		Redland::Node& node = (*i)["node"];
		Redland::Node& type = (*i)["type"];
		if (node.type() == Redland::Node::RESOURCE && type.type() == Redland::Node::RESOURCE) {
			types.insert(make_pair(node.to_string(), type.to_string()));
			if (patches.find(type.to_string()) != patches.end()) {
				patch_nodes.insert(make_pair(node.to_string(), Properties()));
				resources.insert(make_pair(type.to_string(), type));
			} else {
				plugin_nodes.insert(make_pair(node.to_string(), Properties()));
			}
		}
	}


	/* Load nodes on this patch */
	query = Redland::Query(*world->rdf_world, Glib::ustring(
		"SELECT DISTINCT ?node ?predicate ?object WHERE {\n") +
			subject + " ingen:node ?node .\n"
			"?node      ?predicate ?object .\n"
		"}");
	results = query.run(*world->rdf_world, model, base_uri);
	for (Results::iterator i = results.begin(); i != results.end(); ++i) {
		Glib::Mutex::Lock lock(world->rdf_world->mutex());
		Redland::Node&    node      = (*i)["node"];
		Redland::Node&    predicate = (*i)["predicate"];
		Redland::Node&    object    = (*i)["object"];
		Objects::iterator patch_i   = patch_nodes.find(node.to_string());
		Objects::iterator plug_i    = plugin_nodes.find(node.to_string());
		Types::iterator   type_i    = types.find(node.to_string());
		if (node.type() == Redland::Node::RESOURCE && type_i != types.end()) {
			if (patch_i != patch_nodes.end()) {
				patch_i->second.insert(make_pair(predicate.to_string(), object));
			} else if (plug_i != plugin_nodes.end()) {
				plug_i->second.insert(make_pair(predicate.to_string(), object));
			} else {
				cerr << "WARNING: Unrecognized node: " << node.to_string() << endl;
			}
		}
	}

	/* Create patch nodes */
	for (Objects::iterator i = patch_nodes.begin(); i != patch_nodes.end(); ++i) {
		const Path      node_path = Path(relative_uri(base_uri, i->first, true));
		Types::iterator type_i    = types.find(i->first);
		if (type_i == types.end())
			continue;
		Resources::iterator res_i = resources.find(type_i->second);
		if (res_i == resources.end())
			continue;
		parse_patch(world, target, model, res_i->second, patch_path, Symbol(node_path.name()));
		Glib::Mutex::Lock lock(world->rdf_world->mutex());
		for (Properties::iterator j = i->second.begin(); j != i->second.end(); ++j) {
			const string key = world->rdf_world->prefixes().qualify(j->first);
			target->set_variable(node_path, key, AtomRDF::node_to_atom(j->second));
		}
	}

	/* Create plugin nodes */
	for (Objects::iterator i = plugin_nodes.begin(); i != plugin_nodes.end(); ++i) {
		Types::iterator type_i = types.find(i->first);
		if (type_i == types.end())
			continue;
		const Path node_path(relative_uri(base_uri, i->first, true));
		target->new_node(node_path, type_i->second);
		Glib::Mutex::Lock lock(world->rdf_world->mutex());
		for (Properties::iterator j = i->second.begin(); j != i->second.end(); ++j) {
			const string key = world->rdf_world->prefixes().qualify(j->first);
			target->set_variable(node_path, key, AtomRDF::node_to_atom(j->second));
		}
	}


	/* Load node ports */
	query = Redland::Query(*world->rdf_world, Glib::ustring(
		"SELECT DISTINCT ?node ?port ?key ?val WHERE {\n") +
			subject + " ingen:node ?node .\n"
			"?node      lv2:port   ?port .\n"
			"?port      ?key       ?val .\n"
		"}");
	results = query.run(*world->rdf_world, model, base_uri);
	for (Results::iterator i = results.begin(); i != results.end(); ++i) {
		Glib::Mutex::Lock lock(world->rdf_world->mutex());
		const string node_uri = (*i)["node"].to_string();
		const string port_uri = (*i)["port"].to_string();
		if (port_uri.length() <= node_uri.length()) {
			cerr << "WARNING: Port on " << node_uri << " has bad URI: " << port_uri << endl;
			continue;
		}

		const Path   node_path(relative_uri(base_uri, node_uri, true));
		const Symbol port_sym  = port_uri.substr(node_uri.length() + 1);
		const Path   port_path = node_path.base() + port_sym;
		const string key       = world->rdf_world->prefixes().qualify((*i)["key"].to_string());
		if (key == "ingen:value") {
			target->set_port_value(port_path, AtomRDF::node_to_atom((*i)["val"]));
		} else {
			target->set_variable(port_path, key, AtomRDF::node_to_atom((*i)["val"]));
		}
	}


	/* Find ports on this patch */
	query = Redland::Query(*world->rdf_world, Glib::ustring(
		"SELECT DISTINCT ?port ?type WHERE {\n") +
			subject + " lv2:port       ?port .\n"
			"?port      rdf:instanceOf ?type .\n"
		"}");
	Objects patch_ports;
	results = query.run(*world->rdf_world, model, base_uri);
	for (Results::iterator i = results.begin(); i != results.end(); ++i) {
		Glib::Mutex::Lock lock(world->rdf_world->mutex());
		Redland::Node& port = (*i)["port"];
		Redland::Node& type = (*i)["type"];
		if (port.type() == Redland::Node::RESOURCE && type.type() == Redland::Node::RESOURCE) {
			types.insert(make_pair(port.to_string(), type.to_string()));
			patch_ports.insert(make_pair(port.to_string(), Properties()));
		}
	}


	/* Load patch ports */
	query = Redland::Query(*world->rdf_world, Glib::ustring(
		"SELECT DISTINCT ?port ?key ?val WHERE {\n") +
			subject + " lv2:port ?port .\n"
			"?port      ?key     ?val .\n"
		"}");
	results = query.run(*world->rdf_world, model, base_uri);
	for (Results::iterator i = results.begin(); i != results.end(); ++i) {
		Glib::Mutex::Lock lock(world->rdf_world->mutex());
		const string port_uri  = (*i)["port"].to_string();
		const Path   port_path(Path(relative_uri(base_uri, port_uri, true)));
		const string key       = world->rdf_world->prefixes().qualify((*i)["key"].to_string());
		Objects::iterator ports_i = patch_ports.find(port_uri);
		if (ports_i == patch_ports.end())
			ports_i = patch_ports.insert(make_pair(port_uri, Properties())).first;
		ports_i->second.insert(make_pair(key, (*i)["val"]));
	}

	for (Objects::iterator i = patch_ports.begin(); i != patch_ports.end(); ++i) {
		Glib::Mutex::Lock lock(world->rdf_world->mutex());
		const Path port_path(relative_uri(base_uri, i->first, true));
		Properties::iterator types_begin = i->second.find("rdf:type");
		if (types_begin == i->second.end()) {
			cerr << "WARNING: Patch port has no types" << endl;
			continue;
		}
		Properties::iterator types_end = i->second.upper_bound("rdf:type");
		bool is_input  = false;
		bool is_output = false;
		Redland::Node* type = 0;
		for (Properties::iterator t = types_begin; t != types_end; ++t) {
			if (t->second.to_string() == NS_LV2 "InputPort") {
				is_input = true;
			} else if (t->second.to_string() == NS_LV2 "OutputPort") {
				is_output = true;
			} else if (!type || type->to_string() == t->second.to_string()) {
				type = &t->second;
			} else {
				cerr << "ERROR: Port has several data types" << endl;
				continue;
			}
		}
		if ((is_input && is_output) || !type) {
			cerr << "ERROR: Corrupt patch port" << endl;
			continue;
		}
		const string type_str = world->rdf_world->prefixes().qualify(type->to_string());
		target->new_port(port_path, type_str, 0, is_output);
		for (Properties::iterator j = i->second.begin(); j != i->second.end(); ++j) {
			target->set_property(port_path, j->first, AtomRDF::node_to_atom(j->second));
		}
	}

	parse_connections(world, target, model, subject, "/");
	parse_variables(world, target, model, subject_node, patch_path, data);


	/* Enable */
	query = Redland::Query(*world->rdf_world, Glib::ustring(
		"SELECT DISTINCT ?enabled WHERE {\n")
			+ subject + " ingen:enabled ?enabled .\n"
		"}");

	results = query.run(*world->rdf_world, model, base_uri);
	for (Results::iterator i = results.begin(); i != results.end(); ++i) {
		Glib::Mutex::Lock lock(world->rdf_world->mutex());
		const Redland::Node& enabled_node = (*i)["enabled"];
		if (enabled_node.is_bool() && enabled_node) {
			target->set_variable(patch_path, "ingen:enabled", (bool)true);
			break;
		} else {
			cerr << "WARNING: Unknown type for ingen:enabled" << endl;
		}
	}

	return patch_path;
}


boost::optional<Path>
Parser::parse_node(
		Ingen::Shared::World*                    world,
		Ingen::Shared::CommonInterface*          target,
		Redland::Model&                          model,
		const Redland::Node&                     subject,
		const Raul::Path&                        path,
		boost::optional<GraphObject::Properties> data)
{
	/* Get plugin */
	Redland::Query query(*world->rdf_world, Glib::ustring(
		"SELECT DISTINCT ?plug WHERE {\n")
			+ subject.to_turtle_token() + " rdf:instanceOf ?plug .\n"
		"}");

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
		Ingen::Shared::World*                    world,
		Ingen::Shared::CommonInterface*          target,
		Redland::Model&                          model,
		const Redland::Node&                     subject_node,
		const Raul::Path&                        path,
		boost::optional<GraphObject::Properties> data)
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

	for (Redland::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		Glib::Mutex::Lock lock(world->rdf_world->mutex());
		const string         type     = world->rdf_world->qualify((*i)["type"].to_string());
		const string         datatype = world->rdf_world->qualify((*i)["datatype"].to_string());
		const Redland::Node& val_node = (*i)["value"];

		// TODO: read index for plugin wrapper
		bool is_output = (type == "lv2:OutputPort");
		target->new_port(path, datatype, 0, is_output);

		if (val_node.to_string() != "")
			target->set_port_value(path, AtomRDF::node_to_atom(val_node));
	}

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

	Redland::Query::Results results = query.run(*world->rdf_world, model);
	for (Redland::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		Glib::Mutex::Lock lock(world->rdf_world->mutex());
		string src_path = parent.base() + relative_uri(base_uri, (*i)["src"].to_string(), false);
		string dst_path = parent.base() + relative_uri(base_uri, (*i)["dst"].to_string(), false);
		if (Path::is_valid(src_path) && Path::is_valid(dst_path)) {
			target->connect(src_path, dst_path);
		} else {
			cerr << "ERROR: Invalid path in connection "
				<< src_path << " => " <<  dst_path << endl;
		}
	}

	return true;
}


bool
Parser::parse_variables(
		Ingen::Shared::World*                    world,
		Ingen::Shared::CommonInterface*          target,
		Redland::Model&                          model,
		const Redland::Node&                     subject_node,
		const Raul::Path&                        path,
		boost::optional<GraphObject::Properties> data)
{
	const Glib::ustring& subject = subject_node.to_turtle_token();

	Redland::Query query(*world->rdf_world, Glib::ustring(
		"SELECT DISTINCT ?key ?val WHERE {\n") +
			subject + " ?key ?val .\n"
		"}");

	Redland::Query::Results results = query.run(*world->rdf_world, model);
	for (Redland::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		Glib::Mutex::Lock lock(world->rdf_world->mutex());
		const string         key = world->rdf_world->prefixes().qualify(string((*i)["key"]));
		const Redland::Node& val = (*i)["val"];
		if (key != "")
			target->set_variable(path, key, AtomRDF::node_to_atom(val));
	}

	// Set passed variables last to override any loaded values
	if (data)
		for (GraphObject::Properties::const_iterator i = data.get().begin();
				i != data.get().end(); ++i)
			target->set_variable(path, i->first, i->second);

	return true;
}


} // namespace Serialisation
} // namespace Ingen

