/*
  This file is part of Ingen.
  Copyright 2007-2017 David Robillard <http://drobilla.net/>

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

#include "ingen/Atom.hpp"
#include "ingen/AtomForgeSink.hpp"
#include "ingen/Forge.hpp"
#include "ingen/Interface.hpp"
#include "ingen/Log.hpp"
#include "ingen/Parser.hpp"
#include "ingen/URIMap.hpp"
#include "ingen/URIs.hpp"
#include "ingen/World.hpp"
#include "ingen/paths.hpp"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "serd/serd.h"
#include "sord/sordmm.hpp"
#include "sratom/sratom.h"

#define NS_RDF   "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_RDFS  "http://www.w3.org/2000/01/rdf-schema#"

using namespace std;

namespace Ingen {

std::set<Parser::ResourceRecord>
Parser::find_resources(Sord::World&       world,
                       const std::string& manifest_uri,
                       const Raul::URI&   type_uri)
{
	const Sord::URI  base        (world, manifest_uri);
	const Sord::URI  type        (world, type_uri);
	const Sord::URI  rdf_type    (world, NS_RDF "type");
	const Sord::URI  rdfs_seeAlso(world, NS_RDFS "seeAlso");
	const Sord::Node nil;

	SerdEnv* env = serd_env_new(sord_node_to_serd_node(base.c_obj()));
	Sord::Model model(world, manifest_uri);
	model.load_file(env, SERD_TURTLE, manifest_uri);

	std::set<ResourceRecord> resources;
	for (Sord::Iter i = model.find(nil, rdf_type, type); !i.end(); ++i) {
		const Sord::Node  resource     = i.get_subject();
		const std::string resource_uri = resource.to_c_string();
		std::string       file_path    = "";
		Sord::Iter        f            = model.find(resource, rdfs_seeAlso, nil);
		if (!f.end()) {
			uint8_t* p = serd_file_uri_parse(f.get_object().to_u_string(), NULL);
			file_path = (const char*)p;
			free(p);
		}
		resources.insert(ResourceRecord(resource.to_string(), file_path));
	}

	serd_env_free(env);
	return resources;
}

static std::string
relative_uri(const std::string& base, const std::string& uri, bool leading_slash)
{
	std::string ret;
	if (uri != base) {
		SerdURI base_uri;
		serd_uri_parse((const uint8_t*)base.c_str(), &base_uri);

		SerdURI  normal_base_uri;
		SerdNode normal_base_uri_node = serd_node_new_uri_from_string(
			(const uint8_t*)".", &base_uri, &normal_base_uri);

		std::string normal_base_str((const char*)normal_base_uri_node.buf);

		ret = uri;
		if (uri.length() >= normal_base_str.length()
		    && uri.substr(0, normal_base_str.length()) == normal_base_str) {
			ret = uri.substr(normal_base_str.length());
		}

		serd_node_free(&normal_base_uri_node);
	}

	if (leading_slash && ret[0] != '/') {
		ret = std::string("/").append(ret);
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
skip_property(Ingen::URIs& uris, const Sord::Node& predicate)
{
	return (predicate.to_string() == INGEN__file ||
	        predicate.to_string() == uris.ingen_arc ||
	        predicate.to_string() == uris.ingen_block ||
	        predicate.to_string() == uris.lv2_port);
}

static Properties
get_properties(Ingen::World*     world,
               Sord::Model&      model,
               const Sord::Node& subject,
               Resource::Graph   ctx)
{
	LV2_URID_Map* map    = &world->uri_map().urid_map_feature()->urid_map;
	Sratom*       sratom = sratom_new(map);

	LV2_Atom_Forge forge;
	lv2_atom_forge_init(&forge, map);

	AtomForgeSink out(&forge);

	const Sord::Node nil;
	Properties       props;
	for (Sord::Iter i = model.find(subject, nil, nil); !i.end(); ++i) {
		if (!skip_property(world->uris(), i.get_predicate())) {
			out.clear();
			sratom_read(sratom, &forge, world->rdf_world()->c_obj(),
			            model.c_obj(), i.get_object().c_obj());
			const LV2_Atom* atom = out.atom();
			Atom            atomm;
			atomm = world->forge().alloc(
				atom->size, atom->type, LV2_ATOM_BODY_CONST(atom));
			props.insert(make_pair(Raul::URI(i.get_predicate().to_string()),
			                       Property(atomm, ctx)));
		}
	}

	sratom_free(sratom);
	return props;
}

typedef std::pair<Raul::Path, Properties> PortRecord;

static boost::optional<PortRecord>
get_port(Ingen::World*     world,
         Sord::Model&      model,
         const Sord::Node& subject,
         Resource::Graph   ctx,
         const Raul::Path& parent,
         uint32_t*         index)
{
	const URIs& uris = world->uris();

	// Get all properties
	Properties props = get_properties(world, model, subject, ctx);

	// Get index if requested (for Graphs)
	if (index && ctx == Resource::Graph::INTERNAL) {
		Properties::const_iterator i = props.find(uris.lv2_index);
		if (i == props.end()
		    || i->second.type() != world->forge().Int
		    || i->second.get<int32_t>() < 0) {
			world->log().error(fmt("Port %1% has no valid index\n") % subject);
			return boost::optional<PortRecord>();
		}
		*index = i->second.get<int32_t>();
	}

	// Get symbol
	Properties::const_iterator s = props.find(uris.lv2_symbol);
	std::string                sym;
	if (s != props.end() && s->second.type() == world->forge().String) {
		sym = s->second.ptr<char>();
	} else {
		const std::string subject_str = subject.to_string();
		const size_t      last_slash  = subject_str.find_last_of("/");

		sym = ((last_slash == string::npos)
		       ? subject_str
		       : subject_str.substr(last_slash + 1));
	}

	if (!Raul::Symbol::is_valid(sym)) {
		world->log().error(fmt("Port %1% has invalid symbol `%2%'\n")
		                   % subject % sym);
		return boost::optional<PortRecord>();
	}

	const Raul::Symbol port_sym(sym);
	const Raul::Path   port_path(parent.child(port_sym));

	props.erase(uris.lv2_symbol);  // Don't set symbol property in engine
	return make_pair(port_path, props);
}

static boost::optional<Raul::Path>
parse(
	World*                        world,
	Interface*                    target,
	Sord::Model&                  model,
	const std::string&            base_uri,
	Sord::Node&                   subject,
	boost::optional<Raul::Path>   parent = boost::optional<Raul::Path>(),
	boost::optional<Raul::Symbol> symbol = boost::optional<Raul::Symbol>(),
	boost::optional<Properties>   data   = boost::optional<Properties>());

static boost::optional<Raul::Path>
parse_graph(
	World*                        world,
	Interface*                    target,
	Sord::Model&                  model,
	const std::string&            base_uri,
	const Sord::Node&             subject,
	Resource::Graph               ctx,
	boost::optional<Raul::Path>   parent = boost::optional<Raul::Path>(),
	boost::optional<Raul::Symbol> symbol = boost::optional<Raul::Symbol>(),
	boost::optional<Properties>   data   = boost::optional<Properties>());

static boost::optional<Raul::Path>
parse_block(
	World*                      world,
	Interface*                  target,
	Sord::Model&                model,
	const std::string&          base_uri,
	const Sord::Node&           subject,
	const Raul::Path&           path,
	boost::optional<Properties> data = boost::optional<Properties>());

static bool
parse_properties(
	World*                      world,
	Interface*                  target,
	Sord::Model&                model,
	const Sord::Node&           subject,
	Resource::Graph             ctx,
	const Raul::URI&            uri,
	boost::optional<Properties> data = boost::optional<Properties>());

static bool
parse_arcs(
	World*             world,
	Interface*         target,
	Sord::Model&       model,
	const std::string& base_uri,
	const Sord::Node&  subject,
	const Raul::Path&  graph);

static boost::optional<Raul::Path>
parse_block(Ingen::World*               world,
            Ingen::Interface*           target,
            Sord::Model&                model,
            const std::string&          base_uri,
            const Sord::Node&           subject,
            const Raul::Path&           path,
            boost::optional<Properties> data)
{
	const URIs& uris = world->uris();

	// Try lv2:prototype and old ingen:prototype for backwards compatibility
	const Sord::URI prototype_predicates[] = {
		Sord::URI(*world->rdf_world(), uris.lv2_prototype),
		Sord::URI(*world->rdf_world(), uris.ingen_prototype)
	};

	// Get prototype
	Sord::Node prototype;
	for (const Sord::URI& pred : prototype_predicates) {
		prototype = model.get(subject, pred, Sord::Node());
		if (prototype.is_valid()) {
			break;
		}
	}

	if (!prototype.is_valid()) {
		world->log().error(
			fmt("Block %1% (%2%) missing mandatory lv2:prototype\n") %
			subject.to_string() % path);
		return boost::optional<Raul::Path>();
	}

	const uint8_t* type_uri = (const uint8_t*)prototype.to_c_string();
	if (!serd_uri_string_has_scheme(type_uri) ||
	    !strncmp((const char*)type_uri, "file:", 5)) {
		// Prototype is a file, subgraph
		SerdURI base_uri_parts;
		serd_uri_parse((const uint8_t*)base_uri.c_str(), &base_uri_parts);

		SerdURI  ignored;
		SerdNode sub_uri = serd_node_new_uri_from_string(
			type_uri,
			&base_uri_parts,
			&ignored);

		const std::string sub_uri_str = (const char*)sub_uri.buf;
		const std::string sub_file    = sub_uri_str + "/main.ttl";

		const SerdNode sub_base = serd_node_from_string(
			SERD_URI, (const uint8_t*)sub_file.c_str());

		Sord::Model sub_model(*world->rdf_world(), sub_file);
		SerdEnv* env = serd_env_new(&sub_base);
		sub_model.load_file(env, SERD_TURTLE, sub_file);
		serd_env_free(env);

		Sord::URI sub_node(*world->rdf_world(), sub_file);
		parse_graph(world, target, sub_model, (const char*)sub_base.buf,
		            sub_node, Resource::Graph::INTERNAL,
		            path.parent(), Raul::Symbol(path.symbol()));

		parse_graph(world, target, model, base_uri,
		            subject, Resource::Graph::EXTERNAL,
		            path.parent(), Raul::Symbol(path.symbol()));
	} else {
		// Prototype is non-file URI, plugin
		Properties props = get_properties(
			world, model, subject, Resource::Graph::DEFAULT);
		props.insert(make_pair(uris.rdf_type,
		                       uris.forge.make_urid(uris.ingen_Block)));
		target->put(path_to_uri(path), props);
	}
	return path;
}

static boost::optional<Raul::Path>
parse_graph(Ingen::World*                 world,
            Ingen::Interface*             target,
            Sord::Model&                  model,
            const std::string&            base_uri,
            const Sord::Node&             subject,
            Resource::Graph               ctx,
            boost::optional<Raul::Path>   parent,
            boost::optional<Raul::Symbol> symbol,
            boost::optional<Properties>   data)
{
	const URIs& uris = world->uris();

	const Sord::URI ingen_block(*world->rdf_world(), uris.ingen_block);
	const Sord::URI lv2_port(*world->rdf_world(),    LV2_CORE__port);

	const Sord::Node& graph = subject;
	const Sord::Node  nil;

	string graph_path_str = relative_uri(base_uri, subject.to_string(), true);
	if (parent && symbol) {
		graph_path_str = parent->child(*symbol);
	} else if (parent) {
		graph_path_str = *parent;
	} else {
		graph_path_str = "/";
	}

	if (!symbol) {
		symbol = Raul::Symbol("_");
	}

	if (!Raul::Path::is_valid(graph_path_str)) {
		world->log().error(fmt("Graph %1% has invalid path\n")
		                   %  graph_path_str);
		return boost::optional<Raul::Path>();
	}

	// Create graph
	Raul::Path graph_path(graph_path_str);
	Properties props = get_properties(world, model, subject, ctx);
	target->put(path_to_uri(graph_path), props, ctx);

	// For each port on this graph
	typedef std::map<uint32_t, PortRecord> PortRecords;
	PortRecords ports;
	for (Sord::Iter p = model.find(graph, lv2_port, nil); !p.end(); ++p) {
		Sord::Node port = p.get_object();

		// Get all properties
		uint32_t index = 0;
		boost::optional<PortRecord> port_record = get_port(
			world, model, port, ctx, graph_path, &index);
		if (!port_record) {
			world->log().error(fmt("Invalid port %1%\n") % port);
			return boost::optional<Raul::Path>();
		}

		// Store port information in ports map
		if (ports.find(index) == ports.end()) {
			ports[index] = *port_record;
		} else {
			world->log().error(fmt("Ignored port %1% with duplicate index %2%\n")
			                   % port % index);
		}
	}

	// Create ports in order by index
	for (const auto& p : ports) {
		target->put(path_to_uri(p.second.first),
		            p.second.second,
		            ctx);
	}

	if (ctx != Resource::Graph::INTERNAL) {
		return graph_path;  // Not parsing graph internals, finished now
	}

	// For each block in this graph
	for (Sord::Iter n = model.find(subject, ingen_block, nil); !n.end(); ++n) {
		Sord::Node       node       = n.get_object();
		const Raul::Path block_path = graph_path.child(
			Raul::Symbol(get_basename(node.to_string())));

		// Parse and create block
		parse_block(world, target, model, base_uri, node, block_path,
		            boost::optional<Properties>());

		// For each port on this block
		for (Sord::Iter p = model.find(node, lv2_port, nil); !p.end(); ++p) {
			Sord::Node port = p.get_object();

			Resource::Graph subctx = Resource::Graph::DEFAULT;
			if (!model.find(node,
			                Sord::URI(*world->rdf_world(), uris.rdf_type),
			                Sord::URI(*world->rdf_world(), uris.ingen_Graph)).end()) {
				subctx = Resource::Graph::EXTERNAL;
			}

			// Get all properties
			boost::optional<PortRecord> port_record = get_port(
				world, model, port, subctx, block_path, NULL);
			if (!port_record) {
				world->log().error(fmt("Invalid port %1%\n") % port);
				return boost::optional<Raul::Path>();
			}

			// Create port and/or set all port properties
			target->put(path_to_uri(port_record->first),
			            port_record->second,
			            subctx);
		}
	}

	// Now that all ports and blocks exist, create arcs inside graph
	parse_arcs(world, target, model, base_uri, subject, graph_path);

	return graph_path;
}

static bool
parse_arc(Ingen::World*      world,
          Ingen::Interface*  target,
          Sord::Model&       model,
          const std::string& base_uri,
          const Sord::Node&  subject,
          const Raul::Path&  graph)
{
	const URIs& uris = world->uris();

	const Sord::URI  ingen_tail(*world->rdf_world(), uris.ingen_tail);
	const Sord::URI  ingen_head(*world->rdf_world(), uris.ingen_head);
	const Sord::Node nil;

	Sord::Iter t = model.find(subject, ingen_tail, nil);
	Sord::Iter h = model.find(subject, ingen_head, nil);

	if (t.end()) {
		world->log().error("Arc has no tail");
		return false;
	} else if (h.end()) {
		world->log().error("Arc has no head");
		return false;
	}

	const std::string tail_str = relative_uri(
		base_uri, t.get_object().to_string(), true);
	if (!Raul::Path::is_valid(tail_str)) {
		world->log().error("Arc tail has invalid URI");
		return false;
	}

	const std::string head_str = relative_uri(
		base_uri, h.get_object().to_string(), true);
	if (!Raul::Path::is_valid(head_str)) {
		world->log().error("Arc head has invalid URI");
		return false;
	}

	if (!(++t).end()) {
		world->log().error("Arc has multiple tails");
		return false;
	} else if (!(++h).end()) {
		world->log().error("Arc has multiple heads");
		return false;
	}

	target->connect(graph.child(Raul::Path(tail_str)),
	                graph.child(Raul::Path(head_str)));

	return true;
}

static bool
parse_arcs(Ingen::World*      world,
           Ingen::Interface*  target,
           Sord::Model&       model,
           const std::string& base_uri,
           const Sord::Node&  subject,
           const Raul::Path&  graph)
{
	const Sord::URI  ingen_arc(*world->rdf_world(), world->uris().ingen_arc);
	const Sord::Node nil;

	for (Sord::Iter i = model.find(subject, ingen_arc, nil); !i.end(); ++i) {
		parse_arc(world, target, model, base_uri, i.get_object(), graph);
	}

	return true;
}

static bool
parse_properties(Ingen::World*               world,
                 Ingen::Interface*           target,
                 Sord::Model&                model,
                 const Sord::Node&           subject,
                 Resource::Graph             ctx,
                 const Raul::URI&            uri,
                 boost::optional<Properties> data)
{
	Properties properties = get_properties(world, model, subject, ctx);

	target->put(uri, properties, ctx);

	// Set passed properties last to override any loaded values
	if (data)
		target->put(uri, data.get(), ctx);

	return true;
}

static boost::optional<Raul::Path>
parse(Ingen::World*                 world,
      Ingen::Interface*             target,
      Sord::Model&                  model,
      const std::string&            base_uri,
      Sord::Node&                   subject,
      boost::optional<Raul::Path>   parent,
      boost::optional<Raul::Symbol> symbol,
      boost::optional<Properties>   data)
{
	const URIs& uris = world->uris();

	const Sord::URI  graph_class   (*world->rdf_world(), uris.ingen_Graph);
	const Sord::URI  block_class   (*world->rdf_world(), uris.ingen_Block);
	const Sord::URI  arc_class     (*world->rdf_world(), uris.ingen_Arc);
	const Sord::URI  internal_class(*world->rdf_world(), uris.ingen_Internal);
	const Sord::URI  in_port_class (*world->rdf_world(), LV2_CORE__InputPort);
	const Sord::URI  out_port_class(*world->rdf_world(), LV2_CORE__OutputPort);
	const Sord::URI  lv2_class     (*world->rdf_world(), LV2_CORE__Plugin);
	const Sord::URI  rdf_type      (*world->rdf_world(), uris.rdf_type);
	const Sord::Node nil;

	// Parse explicit subject graph
	if (subject.is_valid()) {
		return parse_graph(world, target, model, base_uri,
		                   subject, Resource::Graph::INTERNAL,
		                   parent, symbol, data);
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
	for (const auto& i : subjects) {
		const Sord::Node&           s     = i.first;
		const std::set<Sord::Node>& types = i.second;
		boost::optional<Raul::Path> ret;
		const Raul::Path rel_path(relative_uri(base_uri, s.to_string(), true));
		const Raul::Path path = parent ? parent->child(rel_path) : rel_path;
		if (types.find(graph_class) != types.end()) {
			ret = parse_graph(world, target, model, base_uri,
			                  s, Resource::Graph::INTERNAL,
			                  parent, symbol, data);
		} else if (types.find(block_class) != types.end()) {
			ret = parse_block(world, target, model, base_uri, s, path, data);
		} else if (types.find(in_port_class) != types.end() ||
		           types.find(out_port_class) != types.end()) {
			parse_properties(world, target, model,
			                 s, Resource::Graph::DEFAULT,
			                 path_to_uri(path), data);
			ret = path;
		} else if (types.find(arc_class) != types.end()) {
			Raul::Path parent_path(parent ? parent.get() : Raul::Path("/"));
			parse_arc(world, target, model, base_uri, s, parent_path);
		} else {
			world->log().error("Subject has no known types\n");
		}
	}

	return boost::optional<Raul::Path>();
}

bool
Parser::parse_file(Ingen::World*                 world,
                   Ingen::Interface*             target,
                   const std::string&            path,
                   boost::optional<Raul::Path>   parent,
                   boost::optional<Raul::Symbol> symbol,
                   boost::optional<Properties>   data)
{
	// Get absolute file path
	std::string file_path = path;
	if (!Glib::path_is_absolute(file_path)) {
		file_path = Glib::build_filename(Glib::get_current_dir(), file_path);
	}

	// Find file to use as manifest
	const bool        is_bundle     = Glib::file_test(file_path, Glib::FILE_TEST_IS_DIR);
	const std::string manifest_path = (is_bundle
	                                   ? Glib::build_filename(file_path, "manifest.ttl")
	                                   : file_path);

	std::string manifest_uri;
	try {
		manifest_uri = Glib::filename_to_uri(manifest_path);
	} catch (const Glib::ConvertError& e) {
		world->log().error(fmt("URI conversion error (%1%)\n") % e.what());
		return false;
	}

	// Find graphs in manifest
	const std::set<ResourceRecord> resources = find_resources(
		*world->rdf_world(), manifest_uri, Raul::URI(INGEN__Graph));

	if (resources.empty()) {
		world->log().error(fmt("No graphs found in %1%\n") % path);
		return false;
	}

	/* Choose the graph to load.  If this is a manifest, then there should only be
	   one, but if this is a graph file, subgraphs will be returned as well.
	   In this case, choose the one with the file URI. */
	std::string uri;
	for (const ResourceRecord& r : resources) {
		if (r.uri == Glib::filename_to_uri(manifest_path)) {
			uri = r.uri;
			file_path = r.filename;
			break;
		}
	}

	if (uri.empty()) {
		// Didn't find a graph with the same URI as the file, use the first
		uri       = (*resources.begin()).uri;
		file_path = (*resources.begin()).filename;
	}

	if (file_path.empty()) {
		// No seeAlso file, use manifest (probably the graph file itself)
		file_path = manifest_path;
	}

	// Initialise parsing environment
	const std::string file_uri  = Glib::filename_to_uri(file_path);
	const uint8_t*    uri_c_str = (const uint8_t*)uri.c_str();
	SerdNode          base_node = serd_node_from_string(SERD_URI, uri_c_str);
	SerdEnv*          env       = serd_env_new(&base_node);

	// Load graph into model
	Sord::Model model(*world->rdf_world(), uri, SORD_SPO|SORD_PSO, false);
	model.load_file(env, SERD_TURTLE, file_uri);
	serd_env_free(env);

	world->log().info(fmt("Loading %1% from %2%\n") % uri % file_path);
	if (parent)
		world->log().info(fmt("Parent: %1%\n") % parent->c_str());
	if (symbol)
		world->log().info(fmt("Symbol: %1%\n") % symbol->c_str());

	Sord::Node subject(*world->rdf_world(), Sord::Node::URI, uri);
	boost::optional<Raul::Path> parsed_path
		= parse(world, target, model, model.base_uri().to_string(),
		        subject, parent, symbol, data);

	if (parsed_path) {
		target->set_property(path_to_uri(*parsed_path),
		                     Raul::URI(INGEN__file),
		                     world->forge().alloc_uri(uri));
		return true;
	} else {
		world->log().warn("Document URI lost\n");
		return false;
	}
}

boost::optional<Raul::URI>
Parser::parse_string(Ingen::World*                     world,
                     Ingen::Interface*                 target,
                     const std::string&                str,
                     const std::string&                base_uri,
                     boost::optional<Raul::Path>       parent,
                     boost::optional<Raul::Symbol>     symbol,
                     boost::optional<Properties> data)
{
	// Load string into model
	Sord::Model model(*world->rdf_world(), base_uri, SORD_SPO|SORD_PSO, false);

	SerdEnv* env = serd_env_new(NULL);
	if (!base_uri.empty()) {
		const SerdNode base = serd_node_from_string(
			SERD_URI, (const uint8_t*)base_uri.c_str());
		serd_env_set_base_uri(env, &base);
	}
	model.load_string(env, SERD_TURTLE, str.c_str(), str.length(), base_uri);

	Raul::URI actual_base((const char*)serd_env_get_base_uri(env, NULL)->buf);
	serd_env_free(env);

	world->log().info(fmt("Parsing string (base %1%)\n") % base_uri);

	Sord::Node subject;
	parse(world, target, model, actual_base, subject, parent, symbol, data);
	return actual_base;
}

} // namespace Ingen
