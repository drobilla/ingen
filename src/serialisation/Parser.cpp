/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <map>
#include <set>
#include <string>
#include <utility>

#include <glibmm/convert.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include <glibmm/ustring.h>

#include "ingen/Interface.hpp"
#include "ingen/Log.hpp"
#include "ingen/URIMap.hpp"
#include "ingen/URIs.hpp"
#include "ingen/World.hpp"
#include "ingen/serialisation/Parser.hpp"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "raul/Atom.hpp"
#include "serd/serd.h"
#include "sord/sordmm.hpp"
#include "sratom/sratom.h"

using namespace std;

typedef set<Sord::Node> RDFNodes;

namespace Ingen {
namespace Serialisation {

static Glib::ustring
relative_uri(Glib::ustring base, const Glib::ustring uri, bool leading_slash)
{
	Glib::ustring ret;
	if (uri != base) {
		SerdURI base_uri;
		serd_uri_parse((const uint8_t*)base.c_str(), &base_uri);

		SerdURI  normal_base_uri;
		SerdNode normal_base_uri_node = serd_node_new_uri_from_string(
			(const uint8_t*)".", &base_uri, &normal_base_uri);

		Glib::ustring normal_base_str((const char*)normal_base_uri_node.buf);

		ret = uri;
		if (uri.length() >= normal_base_str.length()
		    && uri.substr(0, normal_base_str.length()) == normal_base_str) {
			ret = uri.substr(normal_base_str.length());
			if (leading_slash && ret[0] != '/')
				ret = Glib::ustring("/") + ret;
		}

		serd_node_free(&normal_base_uri_node);
	}

	if (leading_slash && ret[0] != '/') {
		ret = Glib::ustring("/").append(ret);
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
	        || predicate.to_string() == "http://drobilla.net/ns/ingen#edge"
	        || predicate.to_string() == LV2_CORE__port);
}

static Resource::Properties
get_properties(Ingen::World*     world,
               Sord::Model&      model,
               const Sord::Node& subject)
{
	SerdChunk       out    = { NULL, 0 };
	LV2_URID_Map*   map    = &world->uri_map().urid_map_feature()->urid_map;
	LV2_URID_Unmap* unmap  = &world->uri_map().urid_unmap_feature()->urid_unmap;
	Sratom*         sratom = sratom_new(map);

	LV2_Atom_Forge forge;
	lv2_atom_forge_init(&forge, map);
	lv2_atom_forge_set_sink(&forge, sratom_forge_sink, sratom_forge_deref, &out);

	const Sord::Node nil;
	Resource::Properties props;
	for (Sord::Iter i = model.find(subject, nil, nil); !i.end(); ++i) {
		if (!skip_property(i.get_predicate())) {
			out.len = 0;
			sratom_read(sratom, &forge, world->rdf_world()->c_obj(),
			            model.c_obj(), i.get_object().c_obj());
			const LV2_Atom* atom = (const LV2_Atom*)out.buf;
			Raul::Atom      atomm;
			// FIXME: Don't bloat out all URIs
			if (atom->type == forge.URID) {
				atomm = world->forge().alloc_uri(
					unmap->unmap(unmap->handle,
					             ((const LV2_Atom_URID*)atom)->body));
			} else {
				atomm = world->forge().alloc(
					atom->size, atom->type, LV2_ATOM_BODY_CONST(atom));
			}
			props.insert(make_pair(i.get_predicate().to_string(), atomm));
		}
	}

	free((uint8_t*)out.buf);
	sratom_free(sratom);
	return props;
}

typedef std::pair<Raul::Path, Resource::Properties> PortRecord;

static boost::optional<PortRecord>
get_port(Ingen::World*     world,
         Sord::Model&      model,
         const Sord::Node& subject,
         const Raul::Path& parent,
         uint32_t&         index)
{
	const URIs& uris = world->uris();

	// Get all properties
	Resource::Properties props = get_properties(world, model, subject);

	// Get index
	Resource::Properties::const_iterator i = props.find(uris.lv2_index);
	if (i == props.end()
	    || i->second.type() != world->forge().Int
	    || i->second.get_int32() < 0) {
		world->log().warn(Raul::fmt("Port %1% has no valid index\n") % subject);
		return boost::optional<PortRecord>();
	}
	index = i->second.get_int32();

	// Get symbol
	Resource::Properties::const_iterator s = props.find(uris.lv2_symbol);
	if (s == props.end()) {
		world->log().warn(Raul::fmt("Port %1% has no symbol\n") % subject);
		return boost::optional<PortRecord>();
	}
	const Raul::Symbol port_sym(s->second.get_string());
	const Raul::Path   port_path = parent.child(port_sym);

	return make_pair(port_path, props);
}

static boost::optional<Raul::Path>
parse(
	World*                                world,
	Interface*                            target,
	Sord::Model&                          model,
	Glib::ustring                         document_uri,
	Sord::Node&                           subject,
	boost::optional<Raul::Path>           parent = boost::optional<Raul::Path>(),
	boost::optional<Raul::Symbol>         symbol = boost::optional<Raul::Symbol>(),
	boost::optional<Resource::Properties> data   = boost::optional<Resource::Properties>());

static boost::optional<Raul::Path>
parse_graph(
	World*                                world,
	Interface*                            target,
	Sord::Model&                          model,
	const Sord::Node&                     subject,
	boost::optional<Raul::Path>           parent = boost::optional<Raul::Path>(),
	boost::optional<Raul::Symbol>         symbol = boost::optional<Raul::Symbol>(),
	boost::optional<Resource::Properties> data   = boost::optional<Resource::Properties>());

static boost::optional<Raul::Path>
parse_block(
	World*                                world,
	Interface*                            target,
	Sord::Model&                          model,
	const Sord::Node&                     subject,
	const Raul::Path&                     path,
	boost::optional<Resource::Properties> data = boost::optional<Resource::Properties>());

static bool
parse_properties(
	World*                                world,
	Interface*                            target,
	Sord::Model&                          model,
	const Sord::Node&                     subject,
	const Raul::URI&                      uri,
	boost::optional<Resource::Properties> data = boost::optional<Resource::Properties>());

static bool
parse_edges(
		World*            world,
		Interface*        target,
		Sord::Model&      model,
		const Sord::Node& subject,
		const Raul::Path& graph);

static boost::optional<Raul::Path>
parse_block(Ingen::World*                     world,
            Ingen::Interface*                 target,
            Sord::Model&                      model,
            const Sord::Node&                 subject,
            const Raul::Path&                 path,
            boost::optional<Node::Properties> data)
{
	const URIs& uris = world->uris();

	const Sord::URI  ingen_prototype(*world->rdf_world(), uris.ingen_prototype);
	const Sord::Node nil;

	Sord::Iter i = model.find(subject, ingen_prototype, nil);
	if (i.end() || i.get_object().type() != Sord::Node::URI) {
		world->log().error("BLock missing mandatory ingen:prototype\n");
		return boost::optional<Raul::Path>();
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

		const SerdNode sub_base = serd_node_from_string(
			SERD_URI, (const uint8_t*)sub_file.c_str());

		Sord::Model sub_model(*world->rdf_world(), sub_file);
		SerdEnv* env = serd_env_new(&sub_base);
		sub_model.load_file(env, SERD_TURTLE, sub_file);
		serd_env_free(env);

		Sord::URI sub_node(*world->rdf_world(), sub_file);
		parse_graph(world, target, sub_model, sub_node,
		            path.parent(), Raul::Symbol(path.symbol()));

		parse_graph(world, target, model, subject,
		            path.parent(), Raul::Symbol(path.symbol()));
	} else {
		Resource::Properties props = get_properties(world, model, subject);
		props.insert(make_pair(uris.rdf_type,
		                       uris.forge.alloc_uri(uris.ingen_Block)));
		target->put(Node::path_to_uri(path), props);
	}
	return path;
}

static boost::optional<Raul::Path>
parse_graph(Ingen::World*                     world,
            Ingen::Interface*                 target,
            Sord::Model&                      model,
            const Sord::Node&                 subject_node,
            boost::optional<Raul::Path>       parent,
            boost::optional<Raul::Symbol>     a_symbol,
            boost::optional<Node::Properties> data)
{
	URIs& uris = world->uris();

	const Sord::URI ingen_block(*world->rdf_world(),     uris.ingen_block);
	const Sord::URI ingen_polyphony(*world->rdf_world(), uris.ingen_polyphony);
	const Sord::URI lv2_port(*world->rdf_world(),        LV2_CORE__port);

	const Sord::Node& graph = subject_node;
	const Sord::Node  nil;

	const Glib::ustring base_uri = model.base_uri().to_string();

	Raul::Symbol symbol("_");
	if (a_symbol) {
		symbol = *a_symbol;
	} else {
		const std::string basename = get_basename(base_uri);
	}

	string graph_path_str = relative_uri(base_uri, subject_node.to_string(), true);
	if (parent && a_symbol)
		graph_path_str = parent->child(*a_symbol);

	if (!Raul::Path::is_valid(graph_path_str)) {
		world->log().error(Raul::fmt("Graph %1% has invalid path\n")
		                   %  graph_path_str);
		return boost::optional<Raul::Path>();
	}

	// Create graph
	Raul::Path graph_path(graph_path_str);
	Resource::Properties props = get_properties(world, model, subject_node);
	target->put(Node::path_to_uri(graph_path), props);

	// For each block in this graph
	for (Sord::Iter n = model.find(subject_node, ingen_block, nil); !n.end(); ++n) {
		Sord::Node       node       = n.get_object();
		const Raul::Path block_path = graph_path.child(
			Raul::Symbol(get_basename(node.to_string())));

		// Parse and create block
		parse_block(world, target, model, node, block_path,
		            boost::optional<Node::Properties>());

		// For each port on this block
		for (Sord::Iter p = model.find(node, lv2_port, nil); !p.end(); ++p) {
			Sord::Node port = p.get_object();

			// Get all properties
			uint32_t index = 0;
			boost::optional<PortRecord> port_record = get_port(
				world, model, port, block_path, index);
			if (!port_record) {
				world->log().error(Raul::fmt("Invalid port %1%\n") % port);
				return boost::optional<Raul::Path>();
			}

			// Create port and/or set all port properties
			target->put(Node::path_to_uri(port_record->first),
			            port_record->second);
		}
	}

	// For each port on this graph
	typedef std::map<uint32_t, PortRecord> PortRecords;
	PortRecords ports;
	for (Sord::Iter p = model.find(graph, lv2_port, nil); !p.end(); ++p) {
		Sord::Node port = p.get_object();

		// Get all properties
		uint32_t index = 0;
		boost::optional<PortRecord> port_record = get_port(
			world, model, port, graph_path, index);
		if (!port_record) {
			world->log().error(Raul::fmt("Invalid port %1%\n") % port);
			return boost::optional<Raul::Path>();
		}

		// Store port information in ports map
		ports[index] = *port_record;
	}

	// Create ports in order by index
	for (PortRecords::const_iterator i = ports.begin(); i != ports.end(); ++i) {
		target->put(Node::path_to_uri(i->second.first),
		            i->second.second);
	}

	parse_edges(world, target, model, subject_node, graph_path);

	return graph_path;
}

static bool
parse_edge(Ingen::World*     world,
           Ingen::Interface* target,
           Sord::Model&      model,
           const Sord::Node& subject,
           const Raul::Path& parent)
{
	URIs& uris = world->uris();

	const Sord::URI  ingen_tail(*world->rdf_world(), uris.ingen_tail);
	const Sord::URI  ingen_head(*world->rdf_world(), uris.ingen_head);
	const Sord::Node nil;

	Sord::Iter t = model.find(subject, ingen_tail, nil);
	Sord::Iter h = model.find(subject, ingen_head, nil);

	const Glib::ustring& base_uri = model.base_uri().to_string();

	if (t.end()) {
		world->log().error("Edge has no tail");
		return false;
	} else if (h.end()) {
		world->log().error("Edge has no head");
		return false;
	}

	const std::string tail_str = relative_uri(
		base_uri, t.get_object().to_string(), true);
	if (!Raul::Path::is_valid(tail_str)) {
		world->log().error("Edge tail has invalid URI");
		return false;
	}

	const std::string head_str = relative_uri(
		base_uri, h.get_object().to_string(), true);
	if (!Raul::Path::is_valid(head_str)) {
		world->log().error("Edge head has invalid URI");
		return false;
	}

	if (!(++t).end()) {
		world->log().error("Edge has multiple tails");
		return false;
	} else if (!(++h).end()) {
		world->log().error("Edge has multiple heads");
		return false;
	}

	target->connect(Raul::Path(tail_str), Raul::Path(head_str));

	return true;
}

static bool
parse_edges(Ingen::World*     world,
            Ingen::Interface* target,
            Sord::Model&      model,
            const Sord::Node& subject,
            const Raul::Path& parent)
{
	const Sord::URI  ingen_edge(*world->rdf_world(), world->uris().ingen_edge);
	const Sord::Node nil;

	for (Sord::Iter i = model.find(subject, ingen_edge, nil); !i.end(); ++i) {
		parse_edge(world, target, model, i.get_object(), parent);
	}

	return true;
}

static bool
parse_properties(Ingen::World*                     world,
                 Ingen::Interface*                 target,
                 Sord::Model&                      model,
                 const Sord::Node&                 subject,
                 const Raul::URI&                  uri,
                 boost::optional<Node::Properties> data)
{
	Resource::Properties properties = get_properties(world, model, subject);

	target->put(uri, properties);

	// Set passed properties last to override any loaded values
	if (data)
		target->put(uri, data.get());

	return true;
}

static boost::optional<Raul::Path>
parse(Ingen::World*                     world,
      Ingen::Interface*                 target,
      Sord::Model&                      model,
      Glib::ustring                     document_uri,
      Sord::Node&                       subject,
      boost::optional<Raul::Path>       parent,
      boost::optional<Raul::Symbol>     symbol,
      boost::optional<Node::Properties> data)
{
	URIs& uris = world->uris();

	const Sord::URI  graph_class   (*world->rdf_world(), uris.ingen_Graph);
	const Sord::URI  block_class   (*world->rdf_world(), uris.ingen_Block);
	const Sord::URI  edge_class    (*world->rdf_world(), uris.ingen_Edge);
	const Sord::URI  internal_class(*world->rdf_world(), uris.ingen_Internal);
	const Sord::URI  in_port_class (*world->rdf_world(), LV2_CORE__InputPort);
	const Sord::URI  out_port_class(*world->rdf_world(), LV2_CORE__OutputPort);
	const Sord::URI  lv2_class     (*world->rdf_world(), LV2_CORE__Plugin);
	const Sord::URI  rdf_type      (*world->rdf_world(), uris.rdf_type);
	const Sord::Node nil;

	// Parse explicit subject graph
	if (subject.is_valid()) {
		return parse_graph(world, target, model, subject, parent, symbol, data);
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
	for (Subjects::const_iterator i = subjects.begin(); i != subjects.end(); ++i) {
		const Sord::Node&           s     = i->first;
		const std::set<Sord::Node>& types = i->second;
		boost::optional<Raul::Path> ret;
		const Raul::Path path(
			relative_uri( model.base_uri().to_string(), s.to_string(), true));
		if (types.find(graph_class) != types.end()) {
			ret = parse_graph(world, target, model, s, parent, symbol, data);
		} else if (types.find(block_class) != types.end()) {
			ret = parse_block(world, target, model, s, path, data);
		} else if (types.find(in_port_class) != types.end() ||
		           types.find(out_port_class) != types.end()) {
			parse_properties(
				world, target, model, s, Node::path_to_uri(path), data);
			ret = path;
		} else if (types.find(edge_class) != types.end()) {
			Raul::Path parent_path(parent ? parent.get() : Raul::Path("/"));
			parse_edge(world, target, model, s, parent_path);
		} else {
			world->log().error("Subject has no known types\n");
		}
	}

	return boost::optional<Raul::Path>();
}

/** Parse a graph from RDF into a Interface (engine or client).
 * @return whether or not load was successful.
 */
bool
Parser::parse_file(Ingen::World*                     world,
                   Ingen::Interface*                 target,
                   Glib::ustring                     path,
                   boost::optional<Raul::Path>       parent,
                   boost::optional<Raul::Symbol>     symbol,
                   boost::optional<Node::Properties> data)
{
	if (Glib::file_test(path, Glib::FILE_TEST_IS_DIR)) {
		// This is a bundle, append "/name.ttl" to get graph file path
		path = Glib::build_filename(path, get_basename(path) + ".ttl");
	}

	if (!Glib::path_is_absolute(path)) {
		path = Glib::build_filename(Glib::get_current_dir(), path);
	}

	std::string uri;
	try {
		uri = Glib::filename_to_uri(path, "");
	} catch (const Glib::ConvertError& e) {
		world->log().error(Raul::fmt("Path to URI conversion error: %1%\n")
		                   % e.what());
		return false;
	}

	const uint8_t* uri_c_str = (const uint8_t*)uri.c_str();
	SerdNode       base_node = serd_node_from_string(SERD_URI, uri_c_str);
	SerdEnv*       env       = serd_env_new(&base_node);

	// Load graph file into model
	Sord::Model model(*world->rdf_world(), uri, SORD_SPO|SORD_PSO, false);
	model.load_file(env, SERD_TURTLE, uri);

	serd_env_free(env);

	world->log().info(Raul::fmt("Parsing %1%\n") % path);
	if (parent)
		world->log().info(Raul::fmt("Parent: %1%\n") % parent->c_str());
	if (symbol)
		world->log().info(Raul::fmt("Symbol: %1%\n") % symbol->c_str());

	Sord::Node subject(*world->rdf_world(), Sord::Node::URI, uri);
	boost::optional<Raul::Path> parsed_path
		= parse(world, target, model, path, subject, parent, symbol, data);

	if (parsed_path) {
		target->set_property(Node::path_to_uri(*parsed_path),
		                     Raul::URI("http://drobilla.net/ns/ingen#document"),
		                     world->forge().alloc_uri(uri));
	} else {
		world->log().warn("Document URI lost\n");
	}

	return parsed_path;
}

bool
Parser::parse_string(Ingen::World*                     world,
                     Ingen::Interface*                 target,
                     const Glib::ustring&              str,
                     const Glib::ustring&              base_uri,
                     boost::optional<Raul::Path>       parent,
                     boost::optional<Raul::Symbol>     symbol,
                     boost::optional<Node::Properties> data)
{
	// Load string into model
	Sord::Model model(*world->rdf_world(), base_uri, SORD_SPO|SORD_PSO, false);
	const SerdNode base = serd_node_from_string(
		SERD_URI, (const uint8_t*)base_uri.c_str());
	SerdEnv* env = serd_env_new(&base);
	model.load_string(env, SERD_TURTLE, str.c_str(), str.length(), base_uri);
	serd_env_free(env);

	world->log().info(Raul::fmt("Parsing string (base %1%)\n") % base_uri);

	Sord::Node subject;
	return parse(world, target, model, base_uri, subject, parent, symbol, data);
}

} // namespace Serialisation
} // namespace Ingen

