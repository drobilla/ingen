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

#include "ingen/Parser.hpp"

#include "ingen/Atom.hpp"
#include "ingen/AtomForge.hpp"
#include "ingen/Forge.hpp"
#include "ingen/Interface.hpp"
#include "ingen/Log.hpp"
#include "ingen/Properties.hpp"
#include "ingen/Resource.hpp"
#include "ingen/URI.hpp"
#include "ingen/URIMap.hpp"
#include "ingen/URIs.hpp"
#include "ingen/World.hpp"
#include "ingen/paths.hpp"
#include "lv2/atom/atom.h"
#include "lv2/core/lv2.h"
#include "raul/Path.hpp"
#include "raul/Symbol.hpp"
#include "serd/serd.h"
#include "sord/sord.h"
#include "sord/sordmm.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#define NS_RDF   "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_RDFS  "http://www.w3.org/2000/01/rdf-schema#"

namespace ingen {

std::set<Parser::ResourceRecord>
Parser::find_resources(Sord::World& world,
                       const URI&   manifest_uri,
                       const URI&   type_uri)
{
	const Sord::URI  base        (world, manifest_uri.string());
	const Sord::URI  type        (world, type_uri.string());
	const Sord::URI  rdf_type    (world, NS_RDF "type");
	const Sord::URI  rdfs_seeAlso(world, NS_RDFS "seeAlso");
	const Sord::Node nil;

	SerdEnv* env = serd_env_new(sord_node_to_serd_node(base.c_obj()));
	Sord::Model model(world, manifest_uri.string());
	model.load_file(env, SERD_TURTLE, manifest_uri.string());

	std::set<ResourceRecord> resources;
	for (auto i = model.find(nil, rdf_type, type); !i.end(); ++i) {
		const auto  resource = i.get_subject();
		auto        f        = model.find(resource, rdfs_seeAlso, nil);

		std::string file_path;
		if (!f.end()) {
			uint8_t* p = serd_file_uri_parse(f.get_object().to_u_string(), nullptr);
			file_path = reinterpret_cast<const char*>(p);
			serd_free(p);
		}

		resources.insert(ResourceRecord(resource, file_path));
	}

	serd_env_free(env);
	return resources;
}

static boost::optional<raul::Path>
get_path(const URI& base, const URI& uri)
{
	const URI         relative = uri.make_relative(base);
	const std::string uri_str  = "/" + relative.string();
	return raul::Path::is_valid(uri_str) ? raul::Path(uri_str)
	                                     : boost::optional<raul::Path>();
}

static bool
skip_property(ingen::URIs& uris, const Sord::Node& predicate)
{
	return (predicate == INGEN__file ||
	        predicate == uris.ingen_arc ||
	        predicate == uris.ingen_block ||
	        predicate == uris.lv2_port);
}

static Properties
get_properties(ingen::World&                      world,
               Sord::Model&                       model,
               const Sord::Node&                  subject,
               Resource::Graph                    ctx,
               const boost::optional<Properties>& data = {})
{
	AtomForge forge(world.uri_map().urid_map());

	const Sord::Node nil;
	Properties       props;
	for (auto i = model.find(subject, nil, nil); !i.end(); ++i) {
		if (!skip_property(world.uris(), i.get_predicate())) {
			forge.clear();
			forge.read(
				*world.rdf_world(), model.c_obj(), i.get_object().c_obj());
			const LV2_Atom* atom = forge.atom();
			Atom            atomm;
			atomm = Forge::alloc(
				atom->size, atom->type, LV2_ATOM_BODY_CONST(atom));
			props.emplace(i.get_predicate(), Property(atomm, ctx));
		}
	}

	// Replace any properties given in `data`
	if (data) {
		for (const auto& prop : *data) {
			if (ctx == Resource::Graph::DEFAULT ||
			    prop.second.context() == ctx) {
				props.erase(prop.first);
			}
		}

		for (const auto& prop : *data) {
			if (ctx == Resource::Graph::DEFAULT ||
			    prop.second.context() == ctx) {
				props.emplace(prop);
			}
		}
	}

	return props;
}

using PortRecord = std::pair<raul::Path, Properties>;

static boost::optional<PortRecord>
get_port(ingen::World&     world,
         Sord::Model&      model,
         const Sord::Node& subject,
         Resource::Graph   ctx,
         const raul::Path& parent,
         uint32_t*         index)
{
	const URIs& uris = world.uris();

	// Get all properties
	Properties props = get_properties(world, model, subject, ctx);

	// Get index if requested (for Graphs)
	if (index) {
		const auto i = props.find(uris.lv2_index);
		if (i == props.end()
		    || i->second.type() != world.forge().Int
		    || i->second.get<int32_t>() < 0) {
			world.log().error("Port %1% has no valid index\n", subject);
			return {};
		}
		*index = i->second.get<int32_t>();
	}

	// Get symbol
	auto        s = props.find(uris.lv2_symbol);
	std::string sym;
	if (s != props.end() && s->second.type() == world.forge().String) {
		sym = s->second.ptr<char>();
	} else {
		const std::string subject_str = subject.to_string();
		const size_t      last_slash  = subject_str.find_last_of('/');

		sym = ((last_slash == std::string::npos)
		       ? subject_str
		       : subject_str.substr(last_slash + 1));
	}

	if (!raul::Symbol::is_valid(sym)) {
		world.log().error("Port %1% has invalid symbol `%2%'\n", subject, sym);
		return {};
	}

	const raul::Symbol port_sym(sym);
	const raul::Path   port_path(parent.child(port_sym));

	props.erase(uris.lv2_symbol); // Don't set symbol property in engine
	return make_pair(port_path, props);
}

static boost::optional<raul::Path>
parse(
	World&                               world,
	Interface&                           target,
	Sord::Model&                         model,
	const URI&                           base_uri,
	Sord::Node&                          subject,
	const boost::optional<raul::Path>&   parent = boost::optional<raul::Path>(),
	const boost::optional<raul::Symbol>& symbol = boost::optional<raul::Symbol>(),
	const boost::optional<Properties>&   data   = boost::optional<Properties>());

static boost::optional<raul::Path>
parse_graph(
	World&                               world,
	Interface&                           target,
	Sord::Model&                         model,
	const URI&                           base_uri,
	const Sord::Node&                    subject,
	Resource::Graph                      ctx,
	const boost::optional<raul::Path>&   parent = boost::optional<raul::Path>(),
	const boost::optional<raul::Symbol>& symbol = boost::optional<raul::Symbol>(),
	const boost::optional<Properties>&   data   = boost::optional<Properties>());

static boost::optional<raul::Path>
parse_block(
	World&                             world,
	Interface&                         target,
	Sord::Model&                       model,
	const URI&                         base_uri,
	const Sord::Node&                  subject,
	const raul::Path&                  path,
	const boost::optional<Properties>& data = boost::optional<Properties>());

static bool
parse_arcs(
	World&             world,
	Interface&         target,
	Sord::Model&       model,
	const URI&         base_uri,
	const Sord::Node&  subject,
	const raul::Path&  graph);

static boost::optional<raul::Path>
parse_block(ingen::World&                      world,
            ingen::Interface&                  target,
            Sord::Model&                       model,
            const URI&                         base_uri,
            const Sord::Node&                  subject,
            const raul::Path&                  path,
            const boost::optional<Properties>& data)
{
	const URIs& uris = world.uris();

	// Try lv2:prototype and old ingen:prototype for backwards compatibility
	const Sord::URI prototype_predicates[] = {
		Sord::URI(*world.rdf_world(), uris.lv2_prototype),
		Sord::URI(*world.rdf_world(), uris.ingen_prototype)
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
		world.log().error("Block %1% (%2%) missing mandatory lv2:prototype\n",
		                  subject, path);
		return {};
	}

	const auto* type_uri =
	    reinterpret_cast<const uint8_t*>(prototype.to_c_string());

	if (!serd_uri_string_has_scheme(type_uri) ||
	    !strncmp(reinterpret_cast<const char*>(type_uri), "file:", 5)) {
		// Prototype is a file, subgraph
		SerdURI base_uri_parts;
		serd_uri_parse(reinterpret_cast<const uint8_t*>(base_uri.c_str()),
		               &base_uri_parts);

		SerdURI  ignored;
		SerdNode sub_uri = serd_node_new_uri_from_string(
			type_uri,
			&base_uri_parts,
			&ignored);

		const std::string sub_uri_str = reinterpret_cast<const char*>(sub_uri.buf);
		const std::string sub_file    = sub_uri_str + "/main.ttl";

		const SerdNode sub_base = serd_node_from_string(
		    SERD_URI, reinterpret_cast<const uint8_t*>(sub_file.c_str()));

		Sord::Model sub_model(*world.rdf_world(), sub_file);
		SerdEnv* env = serd_env_new(&sub_base);
		sub_model.load_file(env, SERD_TURTLE, sub_file);
		serd_env_free(env);

		Sord::URI sub_node(*world.rdf_world(), sub_file);
		parse_graph(world, target, sub_model, sub_base,
		            sub_node, Resource::Graph::INTERNAL,
		            path.parent(), raul::Symbol(path.symbol()), data);

		parse_graph(world, target, model, base_uri,
		            subject, Resource::Graph::EXTERNAL,
		            path.parent(), raul::Symbol(path.symbol()), data);
	} else {
		// Prototype is non-file URI, plugin
		Properties props = get_properties(
			world, model, subject, Resource::Graph::DEFAULT, data);
		props.emplace(uris.rdf_type, uris.forge.make_urid(uris.ingen_Block));
		target.put(path_to_uri(path), props);
	}
	return path;
}

static boost::optional<raul::Path>
parse_graph(ingen::World&                        world,
            ingen::Interface&                    target,
            Sord::Model&                         model,
            const URI&                           base_uri,
            const Sord::Node&                    subject,
            Resource::Graph                      ctx,
            const boost::optional<raul::Path>&   parent,
            const boost::optional<raul::Symbol>& symbol,
            const boost::optional<Properties>&   data)
{
	const URIs& uris = world.uris();

	const Sord::URI ingen_block(*world.rdf_world(), uris.ingen_block);
	const Sord::URI lv2_port(*world.rdf_world(),    LV2_CORE__port);

	const Sord::Node& graph = subject;
	const Sord::Node  nil;

	// Build graph path and symbol
	raul::Path graph_path{"/"};
	if (parent && symbol) {
		graph_path = parent->child(*symbol);
	} else if (parent) {
		graph_path = *parent;
	}

	// Create graph
	Properties props = get_properties(world, model, subject, ctx, data);
	target.put(path_to_uri(graph_path), props, ctx);

	// For each port on this graph
	using PortRecords = std::map<uint32_t, PortRecord>;
	PortRecords ports;
	for (auto p = model.find(graph, lv2_port, nil); !p.end(); ++p) {
		Sord::Node port = p.get_object();

		// Get all properties
		uint32_t index = 0;
		boost::optional<PortRecord> port_record = get_port(
			world, model, port, ctx, graph_path, &index);
		if (!port_record) {
			world.log().error("Invalid port %1%\n", port);
			return {};
		}

		// Store port information in ports map
		if (ports.find(index) == ports.end()) {
			ports[index] = *port_record;
		} else {
			world.log().error("Ignored port %1% with duplicate index %2%\n",
			                  port, index);
		}
	}

	// Create ports in order by index
	for (const auto& p : ports) {
		target.put(path_to_uri(p.second.first),
		           p.second.second,
		           ctx);
	}

	if (ctx != Resource::Graph::INTERNAL) {
		return {graph_path}; // Not parsing graph internals, finished now
	}

	// For each block in this graph
	for (auto n = model.find(subject, ingen_block, nil); !n.end(); ++n) {
		Sord::Node node     = n.get_object();
		URI        node_uri = node;
		assert(!node_uri.path().empty() && node_uri.path() != "/");
		const raul::Path block_path = graph_path.child(
			raul::Symbol(FilePath(node_uri.path()).stem().string()));

		// Parse and create block
		parse_block(world, target, model, base_uri, node, block_path,
		            boost::optional<Properties>());

		// For each port on this block
		for (auto p = model.find(node, lv2_port, nil); !p.end(); ++p) {
			Sord::Node port = p.get_object();

			Resource::Graph subctx = Resource::Graph::DEFAULT;
			if (!model.find(node,
			                Sord::URI(*world.rdf_world(), uris.rdf_type),
			                Sord::URI(*world.rdf_world(), uris.ingen_Graph)).end()) {
				subctx = Resource::Graph::EXTERNAL;
			}

			// Get all properties
			boost::optional<PortRecord> port_record = get_port(
				world, model, port, subctx, block_path, nullptr);
			if (!port_record) {
				world.log().error("Invalid port %1%\n", port);
				return {};
			}

			// Create port and/or set all port properties
			target.put(path_to_uri(port_record->first),
			           port_record->second,
			           subctx);
		}
	}

	// Now that all ports and blocks exist, create arcs inside graph
	parse_arcs(world, target, model, base_uri, subject, graph_path);

	return {graph_path};
}

static bool
parse_arc(ingen::World&      world,
          ingen::Interface&  target,
          Sord::Model&       model,
          const URI&         base_uri,
          const Sord::Node&  subject,
          const raul::Path&  graph)
{
	const URIs& uris = world.uris();

	const Sord::URI  ingen_tail(*world.rdf_world(), uris.ingen_tail);
	const Sord::URI  ingen_head(*world.rdf_world(), uris.ingen_head);
	const Sord::Node nil;

	auto t = model.find(subject, ingen_tail, nil);
	auto h = model.find(subject, ingen_head, nil);

	if (t.end()) {
		world.log().error("Arc has no tail\n");
		return false;
	}

	if (h.end()) {
		world.log().error("Arc has no head\n");
		return false;
	}

	const boost::optional<raul::Path> tail_path = get_path(
		base_uri, t.get_object());
	if (!tail_path) {
		world.log().error("Arc tail has invalid URI\n");
		return false;
	}

	const boost::optional<raul::Path> head_path = get_path(
		base_uri, h.get_object());
	if (!head_path) {
		world.log().error("Arc head has invalid URI\n");
		return false;
	}

	if (!(++t).end()) {
		world.log().error("Arc has multiple tails\n");
		return false;
	}

	if (!(++h).end()) {
		world.log().error("Arc has multiple heads\n");
		return false;
	}

	target.connect(graph.child(*tail_path), graph.child(*head_path));

	return true;
}

static bool
parse_arcs(ingen::World&      world,
           ingen::Interface&  target,
           Sord::Model&       model,
           const URI&         base_uri,
           const Sord::Node&  subject,
           const raul::Path&  graph)
{
	const Sord::URI  ingen_arc(*world.rdf_world(), world.uris().ingen_arc);
	const Sord::Node nil;

	for (auto i = model.find(subject, ingen_arc, nil); !i.end(); ++i) {
		parse_arc(world, target, model, base_uri, i.get_object(), graph);
	}

	return true;
}

static boost::optional<raul::Path>
parse(ingen::World&                        world,
      ingen::Interface&                    target,
      Sord::Model&                         model,
      const URI&                           base_uri,
      Sord::Node&                          subject,
      const boost::optional<raul::Path>&   parent,
      const boost::optional<raul::Symbol>& symbol,
      const boost::optional<Properties>&   data)
{
	const URIs& uris = world.uris();

	const Sord::URI  graph_class   (*world.rdf_world(), uris.ingen_Graph);
	const Sord::URI  block_class   (*world.rdf_world(), uris.ingen_Block);
	const Sord::URI  arc_class     (*world.rdf_world(), uris.ingen_Arc);
	const Sord::URI  internal_class(*world.rdf_world(), uris.ingen_Internal);
	const Sord::URI  in_port_class (*world.rdf_world(), LV2_CORE__InputPort);
	const Sord::URI  out_port_class(*world.rdf_world(), LV2_CORE__OutputPort);
	const Sord::URI  lv2_class     (*world.rdf_world(), LV2_CORE__Plugin);
	const Sord::URI  rdf_type      (*world.rdf_world(), uris.rdf_type);
	const Sord::Node nil;

	// Parse explicit subject graph
	if (subject.is_valid()) {
		return parse_graph(world, target, model, base_uri,
		                   subject, Resource::Graph::INTERNAL,
		                   parent, symbol, data);
	}

	// Get all subjects and their types (?subject a ?type)
	using Subjects = std::map< Sord::Node, std::set<Sord::Node> >;
	Subjects subjects;
	for (auto i = model.find(subject, rdf_type, nil); !i.end(); ++i) {
		const Sord::Node& rdf_class = i.get_object();

		assert(rdf_class.is_uri());
		const auto s = subjects.find(i.get_subject());
		if (s == subjects.end()) {
			std::set<Sord::Node> types;
			types.insert(rdf_class);
			subjects.emplace(i.get_subject(), types);
		} else {
			s->second.insert(rdf_class);
		}
	}

	// Parse and create each subject
	for (const auto& i : subjects) {
		const Sord::Node&           s     = i.first;
		const std::set<Sord::Node>& types = i.second;
		boost::optional<raul::Path> ret;
		if (types.find(graph_class) != types.end()) {
			ret = parse_graph(world, target, model, base_uri,
			                  s, Resource::Graph::INTERNAL,
			                  parent, symbol, data);
		} else if (types.find(block_class) != types.end()) {
			const raul::Path rel_path(*get_path(base_uri, s));
			const raul::Path path = parent ? parent->child(rel_path) : rel_path;
			ret = parse_block(world, target, model, base_uri, s, path, data);
		} else if (types.find(in_port_class) != types.end() ||
		           types.find(out_port_class) != types.end()) {
			const raul::Path rel_path(*get_path(base_uri, s));
			const raul::Path path = parent ? parent->child(rel_path) : rel_path;
			const Properties properties = get_properties(
				world, model, s, Resource::Graph::DEFAULT, data);
			target.put(path_to_uri(path), properties);
			ret = path;
		} else if (types.find(arc_class) != types.end()) {
			raul::Path parent_path(parent ? parent.get() : raul::Path("/"));
			parse_arc(world, target, model, base_uri, s, parent_path);
		} else {
			world.log().error("Subject has no known types\n");
		}
	}

	return {};
}

bool
Parser::parse_file(ingen::World&                        world,
                   ingen::Interface&                    target,
                   const FilePath&                      path,
                   const boost::optional<raul::Path>&   parent,
                   const boost::optional<raul::Symbol>& symbol,
                   const boost::optional<Properties>&   data)
{
	// Get absolute file path
	FilePath file_path = path;
	if (!file_path.is_absolute()) {
		file_path = std::filesystem::current_path() / file_path;
	}

	// Find file to use as manifest
	const bool     is_bundle = std::filesystem::is_directory(file_path);
	const FilePath manifest_path =
		(is_bundle ? file_path / "manifest.ttl" : file_path);

	URI manifest_uri(manifest_path);

	// Find graphs in manifest
	const std::set<ResourceRecord> resources = find_resources(
		*world.rdf_world(), manifest_uri, URI(INGEN__Graph));

	if (resources.empty()) {
		world.log().error("No graphs found in %1%\n", path);
		return false;
	}

	/* Choose the graph to load.  If this is a manifest, then there should only be
	   one, but if this is a graph file, subgraphs will be returned as well.
	   In this case, choose the one with the file URI. */
	URI uri;
	for (const ResourceRecord& r : resources) {
		if (r.uri == URI(manifest_path)) {
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
	const URI   file_uri  = URI(file_path);
	const auto* uri_c_str = reinterpret_cast<const uint8_t*>(uri.c_str());
	SerdNode    base_node = serd_node_from_string(SERD_URI, uri_c_str);
	SerdEnv*    env       = serd_env_new(&base_node);

	// Load graph into model
	Sord::Model model(*world.rdf_world(), uri.string(), SORD_SPO|SORD_PSO, false);
	model.load_file(env, SERD_TURTLE, file_uri);
	serd_env_free(env);

	world.log().info("Loading %1% from %2%\n", uri, file_path);
	if (parent) {
		world.log().info("Parent: %1%\n", parent->c_str());
	}
	if (symbol) {
		world.log().info("Symbol: %1%\n", symbol->c_str());
	}

	Sord::Node subject(*world.rdf_world(), Sord::Node::URI, uri.string());
	boost::optional<raul::Path> parsed_path
		= parse(world, target, model, model.base_uri(),
		        subject, parent, symbol, data);

	if (parsed_path) {
		target.set_property(path_to_uri(*parsed_path),
		                    URI(INGEN__file),
		                    world.forge().alloc_uri(uri.string()));
		return true;
	}

	world.log().warn("Document URI lost\n");
	return false;
}

boost::optional<URI>
Parser::parse_string(ingen::World&                        world,
                     ingen::Interface&                    target,
                     const std::string&                   str,
                     const URI&                           base_uri,
                     const boost::optional<raul::Path>&   parent,
                     const boost::optional<raul::Symbol>& symbol,
                     const boost::optional<Properties>&   data)
{
	// Load string into model
	Sord::Model model(*world.rdf_world(), base_uri, SORD_SPO|SORD_PSO, false);

	SerdEnv* env = serd_env_new(nullptr);
	if (!base_uri.empty()) {
		const SerdNode base = serd_node_from_string(
			SERD_URI, reinterpret_cast<const uint8_t*>(base_uri.c_str()));
		serd_env_set_base_uri(env, &base);
	}
	model.load_string(env, SERD_TURTLE, str.c_str(), str.length(), base_uri);

	URI actual_base(reinterpret_cast<const char*>(
	    serd_env_get_base_uri(env, nullptr)->buf));

	serd_env_free(env);

	world.log().info("Parsing string (base %1%)\n", base_uri);

	Sord::Node subject;
	parse(world, target, model, actual_base, subject, parent, symbol, data);
	return {actual_base};
}

} // namespace ingen
