/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
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

#include <locale.h>

#include <set>

#include <boost/format.hpp>

#include <glibmm/convert.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include <glibmm/ustring.h>

#include "raul/Atom.hpp"
#include "raul/AtomRDF.hpp"
#include "raul/TableImpl.hpp"
#include "raul/log.hpp"

#include "serd/serd.h"
#include "sord/sordmm.hpp"

#include "ingen/ServerInterface.hpp"
#include "shared/World.hpp"
#include "shared/LV2URIMap.hpp"

#include "ingen/serialisation/Parser.hpp"

#define LOG(s) s << "[Parser] "

//#define NS_INGEN "http://drobilla.net/ns/ingen#" // defined in Resource.hpp
#define NS_LV2   "http://lv2plug.in/ns/lv2core#"
#define NS_RDF   "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_RDFS  "http://www.w3.org/2000/01/rdf-schema#"

using namespace std;
using namespace Raul;
using namespace Ingen::Shared;

static const Sord::Node nil;

typedef set<Sord::Node> RDFNodes;

namespace Ingen {
namespace Serialisation {

static Glib::ustring
relative_uri(Glib::ustring base, const Glib::ustring uri, bool leading_slash)
{
	if (uri == base) {
		return leading_slash ? "/" : "";
	}

	SerdURI base_uri;
	serd_uri_parse((const uint8_t*)base.c_str(), &base_uri);

	SerdURI  normal_base_uri;
	SerdNode normal_base_uri_node = serd_node_new_uri_from_string(
		(const uint8_t*)".", &base_uri, &normal_base_uri);

	Glib::ustring normal_base_str((const char*)normal_base_uri_node.buf);

	Glib::ustring ret = uri;
	if (uri.length() >= normal_base_str.length()
	    && uri.substr(0, normal_base_str.length()) == normal_base_str) {
		ret = uri.substr(normal_base_str.length());
		if (leading_slash && ret[0] != '/')
			ret = Glib::ustring("/") + ret;
	}

	return ret;
}

static std::string
get_basename(const std::string& uri)
{
	std::string ret = Glib::path_get_basename(uri);
	ret = ret.substr(0, ret.find_last_of('.'));
	return ret;
}

static bool
skip_property(const Sord::Node& predicate)
{
	return (predicate.to_string() == "http://drobilla.net/ns/ingen#node"
	        || predicate.to_string() == "http://drobilla.net/ns/ingen#connection"
	        || predicate.to_string() == "http://lv2plug.in/ns/lv2core#port");
}

static Resource::Properties
get_properties(Sord::Model&      model,
               const Sord::Node& subject)
{
	Resource::Properties props;
	for (Sord::Iter i = model.find(subject, nil, nil); !i.end(); ++i) {
		const Sord::Node& predicate = i.get_predicate();
		const Sord::Node& object    = i.get_object();
		if (!skip_property(predicate)) {
			props.insert(
				make_pair(predicate.to_string(),
				          AtomRDF::node_to_atom(model, object)));
		}
	}
	return props;
}

typedef std::pair<Path, Resource::Properties> PortRecord;

static int
get_port(Ingen::Shared::World* world,
         Sord::Model&          model,
         const Sord::Node&     subject,
         const Raul::Path&     parent,
         PortRecord&           record)
{
	const LV2URIMap& uris = *world->uris().get();

	// Get all properties
	Resource::Properties props = get_properties(model, subject);

	// Get index
	Resource::Properties::const_iterator i = props.find(uris.lv2_index);
	if (i == props.end()
	    || i->second.type() != Atom::INT
	    || i->second.get_int32() < 0) {
		LOG(error) << "Port " << subject << " has no valid lv2:index" << endl;
		return -1;
	}
	const uint32_t index = i->second.get_int32();

	// Get symbol
	Resource::Properties::const_iterator s = props.find(uris.lv2_symbol);
	if (s == props.end()) {
		LOG(error) << "Port " << subject << " has no symbol" << endl;
		return -1;
	}
	const Symbol port_sym  = s->second.get_string();
	const Path   port_path = parent.child(port_sym);

	record = make_pair(port_path, props);
	return index;
}

static boost::optional<Raul::Path>
parse(
	Shared::World*                world,
	CommonInterface*              target,
	Sord::Model&                  model,
	Glib::ustring                 document_uri,
	boost::optional<Raul::Path>   data_path = boost::optional<Raul::Path>(),
	boost::optional<Raul::Path>   parent    = boost::optional<Raul::Path>(),
	boost::optional<Raul::Symbol> symbol    = boost::optional<Raul::Symbol>(),
	boost::optional<Resource::Properties>   data      = boost::optional<Resource::Properties>());

static boost::optional<Raul::Path>
parse_patch(
	Shared::World*                world,
	CommonInterface*              target,
	Sord::Model&                  model,
	const Sord::Node&             subject,
	boost::optional<Raul::Path>   parent = boost::optional<Raul::Path>(),
	boost::optional<Raul::Symbol> symbol = boost::optional<Raul::Symbol>(),
	boost::optional<Resource::Properties>   data   = boost::optional<Resource::Properties>());


static boost::optional<Raul::Path>
parse_node(
	Shared::World*              world,
	CommonInterface*            target,
	Sord::Model&                model,
	const Sord::Node&           subject,
	const Raul::Path&           path,
	boost::optional<Resource::Properties> data = boost::optional<Resource::Properties>());


static bool
parse_properties(
	Shared::World*              world,
	CommonInterface*            target,
	Sord::Model&                model,
	const Sord::Node&           subject,
	const Raul::URI&            uri,
	boost::optional<Resource::Properties> data = boost::optional<Resource::Properties>());

static bool
parse_connections(
		Shared::World*    world,
		CommonInterface*  target,
		Sord::Model&      model,
		const Sord::Node& subject,
		const Raul::Path& patch);

static boost::optional<Path>
parse_node(Ingen::Shared::World*                    world,
           Ingen::CommonInterface*                  target,
           Sord::Model&                             model,
           const Sord::Node&                        subject,
           const Raul::Path&                        path,
           boost::optional<GraphObject::Properties> data)
{
	const LV2URIMap& uris = *world->uris().get();

	Sord::URI rdf_instanceOf(*world->rdf_world(), NS_RDF "instanceOf");

	Sord::Iter i = model.find(subject, rdf_instanceOf, nil);
	if (i.end() || i.get_object().type() != Sord::Node::URI) {
		LOG(error) << "Node missing mandatory rdf:instanceOf" << endl;
		return boost::optional<Path>();
	}

	const std::string type_uri = relative_uri(
		model.base_uri().to_string(),
		i.get_object().to_string(),
		false);

	if (!serd_uri_string_has_scheme((const uint8_t*)type_uri.c_str())) {
		SerdURI base_uri;
		serd_uri_parse(model.base_uri().to_u_string(), &base_uri);

		SerdURI  ignored;
		SerdNode sub_uri = serd_node_new_uri_from_string(
			i.get_object().to_u_string(),
			&base_uri,
			&ignored);

		const std::string sub_uri_str((const char*)sub_uri.buf);
		std::string basename = get_basename(sub_uri_str);

		const std::string sub_file =
			string((const char*)sub_uri.buf) + "/" + basename + ".ttl";

		Sord::Model sub_model(*world->rdf_world(), sub_file);
		SerdEnv* env = serd_env_new(NULL);
		sub_model.load_file(env, SERD_TURTLE, sub_file);
		serd_env_free(env);

		Sord::URI sub_node(*world->rdf_world(), sub_file);
		parse_patch(world, target, sub_model, sub_node,
		            path.parent(), Raul::Symbol(path.symbol()));

		parse_patch(world, target, model, subject,
		            path.parent(), Raul::Symbol(path.symbol()));
	} else {
		Resource::Properties props = get_properties(model, subject);
		props.insert(make_pair(uris.rdf_type,
		                       Raul::URI(uris.ingen_Node)));
		target->put(path, props);
	}
	return path;
}

static boost::optional<Path>
parse_patch(Ingen::Shared::World*                    world,
            Ingen::CommonInterface*                  target,
            Sord::Model&                             model,
            const Sord::Node&                        subject_node,
            boost::optional<Raul::Path>              parent,
            boost::optional<Raul::Symbol>            a_symbol,
            boost::optional<GraphObject::Properties> data)
{
	const Sord::URI ingen_node(*world->rdf_world(),      NS_INGEN "node");
	const Sord::URI ingen_polyphony(*world->rdf_world(), NS_INGEN "polyphony");
	const Sord::URI lv2_port(*world->rdf_world(),        NS_LV2 "port");

	const LV2URIMap&  uris  = *world->uris().get();
	const Sord::Node& patch = subject_node;

	uint32_t patch_poly = 0;

	// Use parameter overridden polyphony, if given
	if (data) {
		GraphObject::Properties::iterator poly_param = data.get().find(uris.ingen_polyphony);
		if (poly_param != data.get().end() && poly_param->second.type() == Atom::INT)
			patch_poly = poly_param->second.get_int32();
	}

	// Load polyphony from file if necessary
	if (patch_poly == 0) {
		Sord::Iter i = model.find(subject_node, ingen_polyphony, nil);
		if (!i.end()) {
			const Sord::Node& poly_node = i.get_object();
			if (poly_node.is_int())
				patch_poly = poly_node.to_int();
			else
				LOG(warn) << "Patch has non-integer polyphony, assuming 1" << endl;
		}
	}

	// No polyphony found anywhere, use 1
	if (patch_poly == 0)
		patch_poly = 1;

	const Glib::ustring base_uri = model.base_uri().to_string();

	Raul::Symbol symbol = "_";
	if (a_symbol) {
		symbol = *a_symbol;
	} else {
		const std::string basename = get_basename(base_uri);
	}

	string patch_path_str = relative_uri(base_uri, subject_node.to_string(), true);
	if (parent && a_symbol)
		patch_path_str = parent->child(*a_symbol).str();

	if (!Path::is_valid(patch_path_str)) {
		LOG(error) << "Patch has invalid path: " << patch_path_str << endl;
		return boost::optional<Raul::Path>();
	}

	// Create patch
	Path patch_path(patch_path_str);
	Resource::Properties props = get_properties(model, subject_node);
	target->put(patch_path, props);

	// For each node in this patch
	for (Sord::Iter n = model.find(subject_node, ingen_node, nil); !n.end(); ++n) {
		Sord::Node node      = n.get_object();
		const Path node_path = patch_path.child(get_basename(node.to_string()));

		// Parse and create node
		parse_node(world, target, model, node, node_path,
		           boost::optional<GraphObject::Properties>());

		// For each port on this node
		for (Sord::Iter p = model.find(node, lv2_port, nil); !p.end(); ++p) {
			Sord::Node port = p.get_object();

			// Get all properties
			PortRecord port_record;
			const int  index = get_port(world, model, port, node_path, port_record);
			if (index < 0) {
				LOG(error) << "Invalid port " << port << endl;
				return boost::optional<Path>();
			}

			// Create port and/or set all port properties
			target->put(port_record.first, port_record.second);
		}
	}

	// For each port on this patch
	typedef std::map<uint32_t, PortRecord> PortRecords;
	PortRecords ports;
	for (Sord::Iter p = model.find(patch, lv2_port, nil); !p.end(); ++p) {
		Sord::Node port = p.get_object();

		// Get all properties
		PortRecord port_record;
		const int  index = get_port(world, model, port, patch_path, port_record);
		if (index < 0) {
			LOG(error) << "Invalid port " << port << endl;
			return boost::optional<Path>();
		}

		// Store port information in ports map
		ports[index] = port_record;
	}

	// Create ports in order by index
	for (PortRecords::const_iterator i = ports.begin(); i != ports.end(); ++i) {
		target->put(i->second.first, i->second.second);
	}

	parse_connections(world, target, model, subject_node, patch_path);

	return patch_path;
}

static bool
parse_connections(Ingen::Shared::World*   world,
                  Ingen::CommonInterface* target,
                  Sord::Model&            model,
                  const Sord::Node&       subject,
                  const Raul::Path&       parent)
{
	Sord::URI ingen_connection(*world->rdf_world(),  NS_INGEN "connection");
	Sord::URI ingen_source(*world->rdf_world(),      NS_INGEN "source");
	Sord::URI ingen_destination(*world->rdf_world(), NS_INGEN "destination");

	const Glib::ustring& base_uri = model.base_uri().to_string();

	RDFNodes connections;
	for (Sord::Iter i = model.find(subject, ingen_connection, nil); !i.end(); ++i) {
		connections.insert(i.get_object());
	}

	for (RDFNodes::const_iterator i = connections.begin(); i != connections.end(); ++i) {
		Sord::Iter s = model.find(*i, ingen_source, nil);
		Sord::Iter d = model.find(*i, ingen_destination, nil);

		if (s.end()) {
			LOG(error) << "Connection has no source" << endl;
			return false;
		} else if (d.end()) {
			LOG(error) << "Connection has no destination" << endl;
			return false;
		}

		const Path src_path(
			parent.child(relative_uri(base_uri, s.get_object().to_string(), false)));
		const Path dst_path(
			parent.child(relative_uri(base_uri, d.get_object().to_string(), false)));

		if (!(++s).end()) {
			LOG(error) << "Connection has multiple sources" << endl;
			return false;
		} else if (!(++d).end()) {
			LOG(error) << "Connection has multiple destinations" << endl;
			return false;
		}

		target->connect(src_path, dst_path);
	}

	return true;
}

static bool
parse_properties(Ingen::Shared::World*                    world,
                 Ingen::CommonInterface*                  target,
                 Sord::Model&                             model,
                 const Sord::Node&                        subject,
                 const Raul::URI&                         uri,
                 boost::optional<GraphObject::Properties> data)
{
	Resource::Properties properties;
	for (Sord::Iter i = model.find(subject, nil, nil); !i.end(); ++i) {
		const Sord::Node& key = i.get_predicate();
		const Sord::Node& val = i.get_object();
		if (!skip_property(key)) {
			properties.insert(make_pair(key.to_string(),
			                            AtomRDF::node_to_atom(model, val)));
		}
	}

	target->put(uri, properties);

	// Set passed properties last to override any loaded values
	if (data)
		target->put(uri, data.get());

	return true;
}

static boost::optional<Path>
parse(Ingen::Shared::World*                    world,
      Ingen::CommonInterface*                  target,
      Sord::Model&                             model,
      Glib::ustring                            document_uri,
      boost::optional<Raul::Path>              data_path,
      boost::optional<Raul::Path>              parent,
      boost::optional<Raul::Symbol>            symbol,
      boost::optional<GraphObject::Properties> data)
{
	const Sord::URI patch_class   (*world->rdf_world(), NS_INGEN "Patch");
	const Sord::URI node_class    (*world->rdf_world(), NS_INGEN "Node");
	const Sord::URI port_class    (*world->rdf_world(), NS_INGEN "Port");
	const Sord::URI internal_class(*world->rdf_world(), NS_INGEN "Internal");
	const Sord::URI in_port_class (*world->rdf_world(), NS_LV2   "InputPort");
	const Sord::URI out_port_class(*world->rdf_world(), NS_LV2   "OutputPort");
	const Sord::URI lv2_class     (*world->rdf_world(), NS_LV2   "Plugin");
	const Sord::URI rdf_type      (*world->rdf_world(), NS_RDF   "type");

	Sord::Node subject = nil;
	if (data_path && data_path->is_root()) {
		subject = model.base_uri();
	} else if (data_path) {
		subject = Sord::URI(*world->rdf_world(), data_path->chop_start("/"));
	}

	Raul::Path path("/");
	if (data_path) {
		path = *data_path;
	} else if (parent && symbol) {
		path = parent->child(*symbol);
	}

	// Get all subjects and their types (?subject a ?type)
	typedef std::map< Sord::Node, std::set<Sord::Node> > Subjects;
	Subjects subjects;
	for (Sord::Iter i = model.find(subject, rdf_type, nil); !i.end(); ++i) {
		const Sord::Node& subject   = i.get_subject();
		const Sord::Node& rdf_class = i.get_object();

		assert(rdf_class.is_uri());
		Subjects::iterator s = subjects.find(subject);
		if (s == subjects.end()) {
			std::set<Sord::Node> types;
			types.insert(rdf_class);
			subjects.insert(make_pair(subject, types));
		} else {
			s->second.insert(rdf_class);
		}
	}

	// Parse and create each subject
	boost::optional<Path> ret;
	boost::optional<Path> root_path;
	for (Subjects::const_iterator i = subjects.begin(); i != subjects.end(); ++i) {
		const Sord::Node&           subject = i->first;
		const std::set<Sord::Node>& types   = i->second;
		if (types.find(patch_class) != types.end()) {
			ret = parse_patch(world, target, model, subject, parent, symbol, data);
		} else if (types.find(node_class) != types.end()) {
			ret = parse_node(world, target, model,
			                 subject, path.child(subject.to_string()),
			                 data);
		} else if (types.find(port_class) != types.end()) {
			parse_properties(world, target, model, subject, path, data);
			ret = path;
		} else {
			LOG(error) << "Subject has no known types" << endl;
		}

		if (!ret) {
			LOG(error) << "Failed to parse " << path << endl;
			LOG(error) << "Types:" << endl;
			for (std::set<Sord::Node>::const_iterator t = types.begin();
			     t != types.end(); ++t) {
				LOG(error) << " :: " << *t << endl;
				assert((*t).is_uri());
			}
			return boost::optional<Path>();
		}

		if (data_path && subject.to_string() == data_path->str()) {
			root_path = ret;
		}
	}

	return path;
}

Parser::Parser(Ingen::Shared::World& world)
{
}

Parser::PatchRecords
Parser::find_patches(Ingen::Shared::World* world,
                     SerdEnv*              env,
                     const Glib::ustring&  manifest_uri)
{
	const Sord::URI ingen_Patch (*world->rdf_world(), NS_INGEN "Patch");
	const Sord::URI rdf_type    (*world->rdf_world(), NS_RDF   "type");
	const Sord::URI rdfs_seeAlso(*world->rdf_world(), NS_RDFS  "seeAlso");

	Sord::Model model(*world->rdf_world(), manifest_uri);
	model.load_file(env, SERD_TURTLE, manifest_uri);

	std::list<PatchRecord> records;
	for (Sord::Iter i = model.find(nil, rdf_type, ingen_Patch); !i.end(); ++i) {
		const Sord::Node patch = i.get_subject();
		Sord::Iter       f     = model.find(patch, rdfs_seeAlso, nil);
		if (!f.end()) {
			records.push_back(PatchRecord(patch.to_c_string(),
			                              f.get_object().to_c_string()));
		} else {
			LOG(error) << "Patch has no rdfs:seeAlso" << endl;
		}
	}
	return records;
}

/** Parse a patch from RDF into a CommonInterface (engine or client).
 * @return whether or not load was successful.
 */
bool
Parser::parse_file(Ingen::Shared::World*                    world,
                   Ingen::CommonInterface*                  target,
                   Glib::ustring                            file_uri,
                   boost::optional<Raul::Path>              parent,
                   boost::optional<Raul::Symbol>            symbol,
                   boost::optional<GraphObject::Properties> data)
{
	const size_t colon = file_uri.find(":");
	if (colon != Glib::ustring::npos) {
		const Glib::ustring scheme = file_uri.substr(0, colon);
		if (scheme != "file") {
			LOG(error) << (boost::format("Unsupported URI scheme `%1%'")
			               % scheme) << endl;
			return false;
		}
	} else {
		file_uri = Glib::ustring("file://") + file_uri;
	}

	const bool is_bundle = (file_uri.substr(file_uri.length() - 4) != ".ttl");

	if (is_bundle && file_uri[file_uri.length() - 1] != '/') {
		file_uri.append("/");
	}

	SerdNode base_node = serd_node_from_string(
		SERD_URI, (const uint8_t*)file_uri.c_str());
	SerdEnv* env = serd_env_new(&base_node);

	if (file_uri.substr(file_uri.length() - 4) != ".ttl") {
		// Not a Turtle file, try to open it as a bundle
		if (file_uri[file_uri.length() - 1] != '/') {
			file_uri.append("/");
		}
		Parser::PatchRecords records = find_patches(world, env, file_uri + "manifest.ttl");
		if (!records.empty()) {
			file_uri = records.front().file_uri;
		}
	}

	base_node = serd_node_from_string(
		SERD_URI, (const uint8_t*)file_uri.c_str());
	serd_env_set_base_uri(env, &base_node);

	// Load patch file into model
	Sord::Model model(*world->rdf_world(), file_uri);
	model.load_file(env, SERD_TURTLE, file_uri);

	serd_env_free(env);

	LOG(info) << "Parsing " << file_uri << endl;
	if (parent)
		LOG(info) << "Parent: " << *parent << endl;
	if (symbol)
		LOG(info) << "Symbol: " << *symbol << endl;

	boost::optional<Path> parsed_path
		= parse(world, target, model, file_uri, Path("/"), parent, symbol, data);

	if (parsed_path) {
		target->set_property(*parsed_path, "http://drobilla.net/ns/ingen#document",
		                     Atom(Atom::URI, file_uri.c_str()));
	} else {
		LOG(warn) << "Document URI lost" << endl;
	}

	return parsed_path;
}

bool
Parser::parse_string(Ingen::Shared::World*                    world,
                     Ingen::CommonInterface*                  target,
                     const Glib::ustring&                     str,
                     const Glib::ustring&                     base_uri,
                     boost::optional<Raul::Path>              parent,
                     boost::optional<Raul::Symbol>            symbol,
                     boost::optional<GraphObject::Properties> data)
{
	// Load string into model
	Sord::Model model(*world->rdf_world(), base_uri);
	SerdEnv* env = serd_env_new(NULL);
	model.load_string(env, SERD_TURTLE, str.c_str(), str.length(), base_uri);
	serd_env_free(env);

	LOG(info) << "Parsing string";
	if (!base_uri.empty())
		info << " (base " << base_uri << ")";
	info << endl;

	boost::optional<Raul::Path> data_path;
	bool ret = parse(world, target, model, base_uri, data_path, parent, symbol, data);
	Sord::URI subject(*world->rdf_world(), base_uri);
	parse_connections(world, target, model, subject, parent ? *parent : "/");

	return ret;
}

} // namespace Serialisation
} // namespace Ingen

