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

#include <boost/optional/optional.hpp>
#include <boost/optional/optional_io.hpp>

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
#include "ingen/filesystem.hpp"
#include "ingen/paths.hpp"
#include "lv2/atom/atom.h"
#include "lv2/atom/forge.h"
#include "lv2/core/lv2.h"
#include "lv2/urid/urid.h"
#include "raul/Path.hpp"
#include "raul/Symbol.hpp"
#include "serd/serd.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <utility>

#define NS_RDF   "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_RDFS  "http://www.w3.org/2000/01/rdf-schema#"

namespace ingen {

static void
load_file(serd::World& world,
          serd::Model& model,
          serd::Env&   env,
          const URI&   file_uri)
{
	serd::Inserter inserter(model, env);
	serd::Reader reader(world, serd::Syntax::Turtle, {}, inserter.sink(), 4096);

	reader.start_file(file_uri.str());
	reader.read_document();
	reader.finish();
}

std::set<Parser::ResourceRecord>
Parser::find_resources(const URI& manifest_uri, const URI& type_uri)
{
	const auto rdf_type     = serd::make_uri(NS_RDF "type");
	const auto rdfs_seeAlso = serd::make_uri(NS_RDFS "seeAlso");

	serd::World world;
	serd::Env   env{serd::Node(manifest_uri)};
	serd::Model model{world,
	                  serd::ModelFlag::index_SPO | serd::ModelFlag::index_OPS};

	load_file(world, model, env, manifest_uri);

	std::set<ResourceRecord> resources;
	for (const auto& s : model.range({}, rdf_type, type_uri)) {
		const auto&    resource = s.subject();
		const auto     f        = model.find(resource, rdfs_seeAlso, {});
		const FilePath file_path =
		        ((f != model.end() ? serd::file_uri_parse((*f).object().str())
		                           : ""));
		resources.insert(ResourceRecord(resource, file_path));
	}

	return resources;
}

static boost::optional<Raul::Path>
get_path(const URI& base, const URI& uri)
{
	const URI         relative = uri.make_relative(base);
	const std::string uri_str  = relative.string();
	// assert(Raul::Path::is_valid(uri_str));
	return Raul::Path::is_valid(uri_str) ? Raul::Path(uri_str)
	                                     : boost::optional<Raul::Path>();
}

static bool
skip_property(ingen::URIs& uris, const serd::Node& predicate)
{
	return (predicate == INGEN__file ||
	        predicate == uris.ingen_arc ||
	        predicate == uris.ingen_block ||
	        predicate == uris.lv2_port);
}

static Properties
get_properties(ingen::World&                      world,
               serd::Model&                       model,
               const serd::Node&                  subject,
               Resource::Graph                    ctx,
               const boost::optional<Properties>& data = {})
{
	AtomForge forge(world.rdf_world(),
	                world.uri_map().urid_map_feature()->urid_map);

	Properties props;
	for (const auto& s : model.range(subject, {}, {})) {
		if (!skip_property(world.uris(), s.predicate())) {
			auto atom = forge.read(model, s.object());
			props.emplace(s.predicate(),
			              Property(world.forge().alloc(atom), ctx));
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

using PortRecord = std::pair<Raul::Path, Properties>;

static boost::optional<PortRecord>
get_port(ingen::World&     world,
         serd::Model&      model,
         const serd::Node& subject,
         Resource::Graph   ctx,
         const Raul::Path& parent,
         uint32_t*         index)
{
	const URIs& uris = world.uris();

	// Get all properties
	Properties props = get_properties(world, model, subject, ctx);

	// Get index if requested (for Graphs)
	if (index) {
		Properties::const_iterator i = props.find(uris.lv2_index);
		if (i == props.end()
		    || i->second.type() != world.forge().Int
		    || i->second.get<int32_t>() < 0) {
			world.log().error("Port %1% has no valid index\n", subject);
			return boost::optional<PortRecord>();
		}
		*index = i->second.get<int32_t>();
	}

	// Get symbol
	Properties::const_iterator s = props.find(uris.lv2_symbol);
	std::string                sym;
	if (s != props.end() && s->second.type() == world.forge().String) {
		sym = s->second.ptr<char>();
	} else {
		const std::string subject_str{subject.str()};
		const size_t      last_slash{subject_str.find_last_of('/')};

		sym = ((last_slash == std::string::npos)
		       ? subject_str
		       : subject_str.substr(last_slash + 1));
	}

	if (!Raul::Symbol::is_valid(sym)) {
		world.log().error("Port %1% has invalid symbol `%2%'\n", subject, sym);
		return boost::optional<PortRecord>();
	}

	const Raul::Symbol port_sym(sym);
	const Raul::Path   port_path(parent.child(port_sym));

	props.erase(uris.lv2_symbol);  // Don't set symbol property in engine
	return make_pair(port_path, props);
}

static boost::optional<Raul::Path>
parse(
	World&                               world,
	Interface&                           target,
	serd::Model&                         model,
	const URI&                           base_uri,
	const boost::optional<Raul::Path>&   parent = boost::optional<Raul::Path>(),
	const boost::optional<Raul::Symbol>& symbol = boost::optional<Raul::Symbol>(),
	const boost::optional<Properties>&   data   = boost::optional<Properties>());

static boost::optional<Raul::Path>
parse_graph(
	World&                               world,
	Interface&                           target,
	serd::Model&                         model,
	const URI&                           base_uri,
	const serd::Node&                    subject,
	Resource::Graph                      ctx,
	const boost::optional<Raul::Path>&   parent = boost::optional<Raul::Path>(),
	const boost::optional<Raul::Symbol>& symbol = boost::optional<Raul::Symbol>(),
	const boost::optional<Properties>&   data   = boost::optional<Properties>());

static boost::optional<Raul::Path>
parse_block(
	World&                             world,
	Interface&                         target,
	serd::Model&                       model,
	const URI&                         base_uri,
	const serd::Node&                  subject,
	const Raul::Path&                  path,
	const boost::optional<Properties>& data = boost::optional<Properties>());

static bool
parse_arcs(
	World&             world,
	Interface&         target,
	serd::Model&       model,
	const URI&         base_uri,
	const serd::Node&  subject,
	const Raul::Path&  graph);

static boost::optional<Raul::Path>
parse_block(ingen::World&                      world,
            ingen::Interface&                  target,
            serd::Model&                       model,
            const URI&                         base_uri,
            const serd::Node&                  subject,
            const Raul::Path&                  path,
            const boost::optional<Properties>& data)
{
	const URIs& uris = world.uris();

	// Try lv2:prototype and old ingen:prototype for backwards compatibility
	const serd::Node prototype_predicates[] = {uris.lv2_prototype,
	                                           uris.ingen_prototype};

	// Get prototype
	serd::Optional<serd::NodeView> prototype;
	for (const auto& pred : prototype_predicates) {
		prototype = model.get(subject, pred, {});
		if (prototype) {
			break;
		}
	}

	if (!prototype) {
		world.log().error(
			fmt("Block %1% (%2%) missing mandatory lv2:prototype\n",
			    subject, path));
		return boost::optional<Raul::Path>();
	}

	const char* type_uri = prototype->c_str();
	if (!serd_uri_string_has_scheme(type_uri) || !strncmp(type_uri, "file:", 5)) {
		// Prototype is a file, subgraph
		serd::Node sub_uri = serd::make_relative_uri(type_uri, base_uri);

		const URI        sub_file{sub_uri.str().str() + "/main.ttl"};
		const serd::Node sub_base = serd::make_uri(sub_file.c_str());

		serd::Model sub_model(world.rdf_world(),
		                      serd::ModelFlag::index_SPO |
		                              serd::ModelFlag::index_OPS);
		serd::Env env(sub_base);
		load_file(world.rdf_world(), sub_model, env, sub_file);

		parse_graph(world, target, sub_model, sub_base,
		            sub_uri, Resource::Graph::INTERNAL,
		            path.parent(), Raul::Symbol(path.symbol()), data);

		parse_graph(world, target, model, base_uri,
		            subject, Resource::Graph::EXTERNAL,
		            path.parent(), Raul::Symbol(path.symbol()), data);
	} else {
		// Prototype is non-file URI, plugin
		Properties props = get_properties(
			world, model, subject, Resource::Graph::DEFAULT, data);
		props.emplace(uris.rdf_type, uris.forge.make_urid(uris.ingen_Block));
		target.put(path_to_uri(path), props);
	}
	return path;
}

static boost::optional<Raul::Path>
parse_graph(ingen::World&                        world,
            ingen::Interface&                    target,
            serd::Model&                         model,
            const URI&                           base_uri,
            const serd::Node&                    subject,
            Resource::Graph                      ctx,
            const boost::optional<Raul::Path>&   parent,
            const boost::optional<Raul::Symbol>& symbol,
            const boost::optional<Properties>&   data)
{
	const URIs& uris = world.uris();

	const serd::Node lv2_port = serd::make_uri(LV2_CORE__port);

	const serd::Node& graph = subject;

	// Build graph path and symbol
	Raul::Path graph_path;
	if (parent && symbol) {
		graph_path = parent->child(*symbol);
	} else if (parent) {
		graph_path = *parent;
	} else {
		graph_path = Raul::Path("/");
	}

	// Create graph
	Properties props = get_properties(world, model, subject, ctx, data);
	target.put(path_to_uri(graph_path), props, ctx);

	// For each port on this graph
	using PortRecords = std::map<uint32_t, PortRecord>;
	PortRecords ports;
	for (const auto& s : model.range(graph, lv2_port, {})) {
		const auto& port = s.object();

		// Get all properties
		uint32_t index = 0;
		boost::optional<PortRecord> port_record = get_port(
			world, model, port, ctx, graph_path, &index);
		if (!port_record) {
			world.log().error("Invalid port %1%\n", port);
			return boost::optional<Raul::Path>();
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
		return graph_path;  // Not parsing graph internals, finished now
	}

	// For each block in this graph
	for (const auto& b : model.range(subject, uris.ingen_block, {})) {
		const auto& node     = b.object();
		URI         node_uri = node;
		assert(!node_uri.path().empty() && node_uri.path() != "/");
		const Raul::Path block_path = graph_path.child(
			Raul::Symbol(FilePath(node_uri.path()).stem().string()));

		// Parse and create block
		parse_block(world, target, model, base_uri, node, block_path,
		            boost::optional<Properties>());

		// For each port on this block
		for (const auto& p : model.range(node, lv2_port, {})) {
			const auto& port = p.object();

			Resource::Graph subctx = Resource::Graph::DEFAULT;
			if (model.find(node, uris.rdf_type, uris.ingen_Graph) ==
			    model.end()) {
				subctx = Resource::Graph::EXTERNAL;
			}

			// Get all properties
			boost::optional<PortRecord> port_record = get_port(
				world, model, port, subctx, block_path, nullptr);
			if (!port_record) {
				world.log().error("Invalid port %1%\n", port);
				return boost::optional<Raul::Path>();
			}

			// Create port and/or set all port properties
			target.put(path_to_uri(port_record->first),
			           port_record->second,
			           subctx);
		}
	}

	// Now that all ports and blocks exist, create arcs inside graph
	parse_arcs(world, target, model, base_uri, subject, graph_path);

	return graph_path;
}

static bool
parse_arc(ingen::World&      world,
          ingen::Interface&  target,
          serd::Model&       model,
          const URI&         base_uri,
          const serd::Node&  subject,
          const Raul::Path&  graph)
{
	const URIs& uris = world.uris();

	const auto& tail = model.get(subject, uris.ingen_tail, {});
	const auto& head = model.get(subject, uris.ingen_head, {});

	if (!tail) {
		world.log().error("Arc has no tail\n");
		return false;
	} else if (!head) {
		world.log().error("Arc has no head\n");
		return false;
	}

	const boost::optional<Raul::Path> tail_path = get_path(
		base_uri, *tail);
	if (!tail_path) {
		world.log().error("Arc tail has invalid URI\n");
		return false;
	}

	const boost::optional<Raul::Path> head_path = get_path(
		base_uri, *head);
	if (!head_path) {
		world.log().error("Arc head has invalid URI\n");
		return false;
	}

	target.connect(graph.child(*tail_path), graph.child(*head_path));

	return true;
}

static bool
parse_arcs(ingen::World&      world,
           ingen::Interface&  target,
           serd::Model&       model,
           const URI&         base_uri,
           const serd::Node&  subject,
           const Raul::Path&  graph)
{
	for (const auto& s : model.range(subject, world.uris().ingen_arc, {})) {
		parse_arc(world, target, model, base_uri, s.object(), graph);
	}

	return true;
}

static boost::optional<Raul::Path>
parse(ingen::World&                        world,
      ingen::Interface&                    target,
      serd::Model&                         model,
      const URI&                           base_uri,
      const boost::optional<Raul::Path>&   parent,
      const boost::optional<Raul::Symbol>& symbol,
      const boost::optional<Properties>&   data)
{
	using Subjects = std::map<serd::Node, std::set<serd::Node>>;

	const URIs& uris = world.uris();

	// Get all subjects and their types (?subject a ?type)
	Subjects subjects;
	for (const auto& s : model.range({}, uris.rdf_type, {})) {
		const auto& subject   = s.subject();
		const auto& rdf_class = s.object();

		assert(rdf_class.type() == serd::NodeType::URI);
		auto t = subjects.find(subject);
		if (t == subjects.end()) {
			std::set<serd::Node> types;
			types.insert(rdf_class);
			subjects.emplace(subject, types);
		} else {
			t->second.insert(rdf_class);
		}
	}

	// Parse and create each subject
	for (const auto& i : subjects) {
		const auto& s     = i.first;
		const auto& types = i.second;

		boost::optional<Raul::Path> ret;
		if (types.find(uris.ingen_Graph) != types.end()) {
			ret = parse_graph(world, target, model, base_uri,
			                  s, Resource::Graph::INTERNAL,
			                  parent, symbol, data);
		} else if (types.find(uris.ingen_Block) != types.end()) {
			const Raul::Path rel_path(*get_path(base_uri, s));
			const Raul::Path path = parent ? parent->child(rel_path) : rel_path;
			ret = parse_block(world, target, model, base_uri, s, path, data);
		} else if (types.find(uris.lv2_InputPort) != types.end() ||
		           types.find(uris.lv2_OutputPort) != types.end()) {
			const Raul::Path rel_path(*get_path(base_uri, s));
			const Raul::Path path = parent ? parent->child(rel_path) : rel_path;
			const Properties properties = get_properties(
				world, model, s, Resource::Graph::DEFAULT, data);
			target.put(path_to_uri(path), properties);
			ret = path;
		} else if (types.find(uris.ingen_Arc) != types.end()) {
			Raul::Path parent_path(parent ? parent.get() : Raul::Path("/"));
			parse_arc(world, target, model, base_uri, s, parent_path);
		} else {
			world.log().error("Subject has no known types\n");
		}
	}

	return boost::optional<Raul::Path>();
}

bool
Parser::parse_file(ingen::World&                        world,
                   ingen::Interface&                    target,
                   const FilePath&                      path,
                   const boost::optional<Raul::Path>&   parent,
                   const boost::optional<Raul::Symbol>& symbol,
                   const boost::optional<Properties>&   data)
{
	// Get absolute file path
	FilePath file_path = path;
	if (!file_path.is_absolute()) {
		file_path = filesystem::current_path() / file_path;
	}

	// Find file to use as manifest
	const bool     is_bundle = filesystem::is_directory(file_path);
	const FilePath manifest_path =
		(is_bundle ? file_path / "manifest.ttl" : file_path);

	URI manifest_uri(manifest_path);

	// Find graphs in manifest
	const std::set<ResourceRecord> resources =
		find_resources(manifest_uri, URI(INGEN__Graph));

	if (resources.empty()) {
		world.log().error("No graphs found in %1%\n", path);
		return false;
	}

	/* Choose the graph to load.  If this is a manifest, then there should only be
	   one, but if this is a graph file, subgraphs will be returned as well.
	   In this case, choose the one with the file URI. */
	boost::optional<URI> uri;
	for (const ResourceRecord& r : resources) {
		if (r.uri == URI(manifest_path)) {
			uri = r.uri;
			file_path = r.filename;
			break;
		}
	}

	if (!uri) {
		// Didn't find a graph with the same URI as the file, use the first
		uri       = (*resources.begin()).uri;
		file_path = (*resources.begin()).filename;
	}

	if (file_path.empty()) {
		// No seeAlso file, use manifest (probably the graph file itself)
		file_path = manifest_path;
	}

	// Initialise parsing environment
	const URI file_uri(file_path);
	serd::Env env(*uri);

	// Load graph into model
	serd::Model model(world.rdf_world(),
	                  serd::ModelFlag::index_SPO | serd::ModelFlag::index_OPS);
	load_file(world.rdf_world(), model, env, file_uri);

	world.log().info("Loading %1% from %2%\n", uri, file_path);
	if (parent) {
		world.log().info("Parent: %1%\n", parent->c_str());
	}
	if (symbol) {
		world.log().info("Symbol: %1%\n", symbol->c_str());
	}

	boost::optional<Raul::Path> parsed_path =
	        parse_graph(world,
	                    target,
	                    model,
	                    *uri,
	                    *uri,
	                    Resource::Graph::INTERNAL,
	                    parent,
	                    symbol,
	                    data);

	if (parsed_path) {
		target.set_property(path_to_uri(*parsed_path),
		                    URI(INGEN__file),
		                    world.forge().alloc_uri(uri->string()));
		return true;
	} else {
		world.log().warn("Document URI lost\n");
		return false;
	}
}

boost::optional<URI>
Parser::parse_string(ingen::World&                        world,
                     ingen::Interface&                    target,
                     const std::string&                   str,
                     const URI&                           base_uri,
                     const boost::optional<Raul::Path>&   parent,
                     const boost::optional<Raul::Symbol>& symbol,
                     const boost::optional<Properties>&   data)
{
	// Load string into model
	serd::Model model(world.rdf_world(),
	                  serd::ModelFlag::index_SPO | serd::ModelFlag::index_OPS);

	serd::Env env;
	if (!base_uri.empty()) {
		env.set_base_uri(base_uri);
	}

	serd::Inserter inserter(model, env);
	serd::Reader   reader(
		world.rdf_world(), serd::Syntax::Turtle, {}, inserter.sink(), 4096);

	reader.start_string(str);
	reader.read_document();
	reader.finish();

	URI actual_base(env.base_uri());

	world.log().info("Parsing string (base %1%)\n", base_uri);

	parse(world, target, model, actual_base, parent, symbol, data);
	return actual_base;
}

} // namespace ingen
