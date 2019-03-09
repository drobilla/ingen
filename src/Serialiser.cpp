/*
  This file is part of Ingen.
  Copyright 2007-2016 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ingen/Serialiser.hpp"

#include "ingen/Arc.hpp"
#include "ingen/Atom.hpp"
#include "ingen/FilePath.hpp"
#include "ingen/Forge.hpp"
#include "ingen/Log.hpp"
#include "ingen/Node.hpp"
#include "ingen/Resource.hpp"
#include "ingen/Store.hpp"
#include "ingen/URI.hpp"
#include "ingen/URIMap.hpp"
#include "ingen/URIs.hpp"
#include "ingen/World.hpp"
#include "ingen/filesystem.hpp"
#include "ingen/runtime_paths.hpp"
#include "ingen/types.hpp"
#include "lv2/core/lv2.h"
#include "lv2/state/state.h"
#include "lv2/ui/ui.h"
#include "lv2/urid/urid.h"
#include "raul/Path.hpp"
#include "raul/Symbol.hpp"
#include "serd/serd.h"
#include "serd/serd.hpp"
#include "sratom/sratom.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace ingen {

struct Serialiser::Impl {
	explicit Impl(World& world)
	    : _root_path("/")
	    , _mode(Mode::TO_STRING)
	    , _base_uri("ingen:")
	    , _world(world)
	    , _streamer(_world.rdf_world(),
	                _world.uri_map().urid_map_feature()->urid_map,
	                _world.uri_map().urid_unmap_feature()->urid_unmap)
	{
	}

	Impl(const Impl&) = delete;
	Impl(Impl&&) = delete;
	Impl& operator=(const Impl&) = delete;
	Impl& operator=(Impl&&) = delete;

	enum class Mode { TO_FILE, TO_STRING };

	void start_to_file(const Raul::Path& root,
	                   const FilePath&   filename);

	std::set<const Resource*> serialise_graph(const SPtr<const Node>& graph,
	                                          const serd::Node&       graph_id);

	void serialise_block(const SPtr<const Node>& block,
	                     const serd::Node&       class_id,
	                     const serd::Node&       block_id);

	void serialise_port(const Node*       port,
	                    Resource::Graph   context,
	                    const serd::Node& port_id);

	void serialise_properties(serd::Node        id,
	                          const Properties& props);

	void write_bundle(const SPtr<const Node>& graph, const URI& uri);

	serd::Node path_rdf_node(const Raul::Path& path);

	void write_manifest(const FilePath&         bundle_path,
	                    const SPtr<const Node>& graph);

	void write_plugins(const FilePath&                  bundle_path,
	                   const std::set<const Resource*>& plugins);

	void serialise_arc(const serd::Optional<serd::Node>& parent,
	                   const SPtr<const Arc>&            arc);

	std::string finish();

	Raul::Path        _root_path;
	Mode              _mode;
	URI               _base_uri;
	FilePath          _basename;
	World&            _world;
	UPtr<serd::Model> _model;
	sratom::Streamer  _streamer;
};

Serialiser::Serialiser(World& world)
	: me{make_unique<Impl>(world)}
{}

Serialiser::~Serialiser() = default;

void
Serialiser::Impl::write_manifest(const FilePath& bundle_path,
                                 const SPtr<const Node>&)
{
	const FilePath manifest_path(bundle_path / "manifest.ttl");
	const FilePath binary_path(ingen_module_path("lv2"));

	start_to_file(Raul::Path("/"), manifest_path);

	const URIs& uris  = _world.uris();

	const std::string filename("main.ttl");
	const serd::Node  subject = serd::make_resolved_uri(filename, _base_uri);

	_model->insert(subject, uris.rdf_type, uris.ingen_Graph);
	_model->insert(subject, uris.rdf_type, uris.lv2_Plugin);
	_model->insert(subject, uris.lv2_prototype, uris.ingen_GraphPrototype);
	_model->insert(subject,
	               uris.rdfs_seeAlso,
	               serd::make_resolved_uri(filename, _base_uri));

	finish();
}

void
Serialiser::Impl::write_plugins(const FilePath&                  bundle_path,
                                const std::set<const Resource*>& plugins)
{
	const FilePath plugins_path(bundle_path / "plugins.ttl");

	start_to_file(Raul::Path("/"), plugins_path);

	const URIs& uris  = _world.uris();

	for (const auto& p : plugins) {
		const Atom& minor = p->get_property(uris.lv2_minorVersion);
		const Atom& micro = p->get_property(uris.lv2_microVersion);

		_model->insert(p->uri(), uris.rdf_type, uris.lv2_Plugin);

		if (minor.is_valid() && micro.is_valid()) {
			_model->insert(p->uri(),
			               uris.lv2_minorVersion,
			               serd::make_integer(minor.get<int32_t>()));
			_model->insert(p->uri(),
			               uris.lv2_microVersion,
			               serd::make_integer(micro.get<int32_t>()));
		}
	}

	finish();
}

void
Serialiser::write_bundle(const SPtr<const Node>& graph, const URI& uri)
{
	me->write_bundle(graph, uri);
}

void
Serialiser::Impl::write_bundle(const SPtr<const Node>& graph, const URI& uri)
{
	FilePath path(uri.path());
	if (filesystem::exists(path) && !filesystem::is_directory(path)) {
		path = path.parent_path();
	}

	_world.log().info("Writing bundle %1%\n", path);
	filesystem::create_directories(path);

	const FilePath   main_file     = path / "main.ttl";
	const Raul::Path old_root_path = _root_path;

	start_to_file(graph->path(), main_file);

	std::set<const Resource*> plugins = serialise_graph(
		graph,
		serd::make_resolved_uri(main_file.c_str(), _base_uri));

	finish();
	write_manifest(path, graph);
	write_plugins(path, plugins);

	_root_path = old_root_path;
}

/** Begin a serialization to a file.
 *
 * This must be called before any serializing methods.
 */
void
Serialiser::Impl::start_to_file(const Raul::Path& root,
                                const FilePath&   filename)
{
	_base_uri = URI(filename);
	_basename = filename.stem();
	if (_basename == "main") {
		_basename = filename.parent_path().stem();
	}

	_root_path = root;
	_mode      = Mode::TO_FILE;
	_model     = make_unique<serd::Model>(_world.rdf_world(),
	                                      serd::ModelFlag::index_SPO |
	                                      serd::ModelFlag::index_OPS);
}

void
Serialiser::start_to_string(const Raul::Path& root, const URI& base_uri)
{
	me->_root_path = root;
	me->_base_uri  = base_uri;
	me->_mode      = Impl::Mode::TO_STRING;
	me->_model     = make_unique<serd::Model>(me->_world.rdf_world(),
	                                          serd::ModelFlag::index_SPO |
	                                          serd::ModelFlag::index_OPS);
}

void
Serialiser::start_to_file(const Raul::Path& root, const FilePath& filename)
{
	me->start_to_file(root, filename);
}

std::string
Serialiser::finish()
{
	return me->finish();
}

std::string
Serialiser::Impl::finish()
{
	std::string ret{};
	serd::Env   env{_world.env()};

	env.set_base_uri(_base_uri);

	if (_mode == Mode::TO_FILE) {
		const std::string path = serd::file_uri_parse(_base_uri.string());
		std::ofstream     file(path);
		serd::Writer      writer(
			_world.rdf_world(), serd::Syntax::Turtle, {}, env, file);

		env.write_prefixes(writer.sink());
		const serd::Status st = _model->all().serialise(writer.sink());

		if (st != serd::Status::success) {
			_world.log().error(fmt("Error writing file %1% (%2%)\n",
			                       _base_uri, serd::strerror(st)));
		}
	} else {
		std::stringstream ss;
		serd::Writer      writer(
                _world.rdf_world(), serd::Syntax::Turtle, {}, env, ss);

		env.write_prefixes(writer.sink());
		const serd::Status st = _model->all().serialise(writer.sink());

		writer.finish();
		ret = ss.str();

		if (st != serd::Status::success) {
			_world.log().error(fmt("Error writing string (%2%)\n",
			                       serd::strerror(st)));
		}
	}

	_model.reset();
	_base_uri = URI("ingen:");

	return ret;
}

serd::Node
Serialiser::Impl::path_rdf_node(const Raul::Path& path)
{
	assert(_model);
	assert(path == _root_path || path.is_child_of(_root_path));
	return serd::make_resolved_uri(path.substr(_root_path.base().length()),
	                               _base_uri);
}

void
Serialiser::serialise(const SPtr<const Node>& object, Resource::Graph context)
{
	if (!me->_model) {
		throw std::logic_error("serialise called without serialisation in progress");
	}

	if (object->graph_type() == Node::GraphType::GRAPH) {
		me->serialise_graph(object, me->path_rdf_node(object->path()));
	} else if (object->graph_type() == Node::GraphType::BLOCK) {
		me->serialise_block(object,
		                    object->plugin()->uri(),
		                    me->path_rdf_node(object->path()));
	} else if (object->graph_type() == Node::GraphType::PORT) {
		me->serialise_port(
			object.get(), context, me->path_rdf_node(object->path()));
	} else {
		me->serialise_properties(me->path_rdf_node(object->path()),
		                         object->properties());
	}
}

std::set<const Resource*>
Serialiser::Impl::serialise_graph(const SPtr<const Node>& graph,
                                  const serd::Node&       graph_id)
{
	const URIs& uris = _world.uris();

	_model->insert(graph_id, uris.rdf_type, uris.ingen_Graph);

	_model->insert(graph_id, uris.rdf_type, uris.lv2_Plugin);

	_model->insert(graph_id,
	               uris.lv2_extensionData,
	               serd::make_uri(LV2_STATE__interface));

	_model->insert(graph_id,
	               serd::make_uri(LV2_UI__ui),
	               serd::make_uri("http://drobilla.net/ns/ingen#GraphUIGtk2"));

	// If the graph has no doap:name (required by LV2), use the basename
	if (graph->properties().find(uris.doap_name) == graph->properties().end()) {
		_model->insert(graph_id,
		               uris.doap_name,
		               serd::make_string((std::string)_basename));
	}

	const Properties props = graph->properties(Resource::Graph::INTERNAL);
	serialise_properties(graph_id, props);

	std::set<const Resource*> plugins;

	const Store::const_range kids = _world.store()->children_range(graph);
	for (Store::const_iterator n = kids.first; n != kids.second; ++n) {
		if (n->first.parent() != graph->path()) {
			continue;
		}

		if (n->second->graph_type() == Node::GraphType::GRAPH) {
			SPtr<Node> subgraph = n->second;

			const std::string sub_bundle_path = subgraph->path().substr(1) + ".ingen";

			serd::Node subgraph_node = serd::make_resolved_uri(
				sub_bundle_path,
				_base_uri);

			// Save our state
			URI  my_base_uri = _base_uri;
			auto my_model    = std::move(_model);

			// Write child bundle within this bundle
			write_bundle(subgraph, subgraph_node);

			// Restore our state
			_base_uri = my_base_uri;
			_model    = std::move(my_model);

			// Serialise reference to graph block
			const serd::Node block_id(path_rdf_node(subgraph->path()));
			_model->insert(graph_id, uris.ingen_block, block_id);
			serialise_block(subgraph, subgraph_node, block_id);
		} else if (n->second->graph_type() == Node::GraphType::BLOCK) {
			SPtr<const Node> block = n->second;

			const serd::Node block_id(path_rdf_node(n->second->path()));
			_model->insert(graph_id, uris.ingen_block, block_id);
			serialise_block(block, block->plugin()->uri(), block_id);

			plugins.insert(block->plugin());
		}
	}

	for (uint32_t i = 0; i < graph->num_ports(); ++i) {
		Node* p = graph->port(i);
		const serd::Node port_id = path_rdf_node(p->path());

		// Ensure lv2:name always exists so Graph is a valid LV2 plugin
		if (p->properties().find(uris.lv2_name) == p->properties().end()) {
			p->set_property(uris.lv2_name,
			                _world.forge().alloc(p->symbol().c_str()));
		}

		_model->insert(graph_id, serd::make_uri(LV2_CORE__port), port_id);
		serialise_port(p, Resource::Graph::DEFAULT, port_id);
		serialise_port(p, Resource::Graph::INTERNAL, port_id);
	}

	for (const auto& a : graph->arcs()) {
		serialise_arc(graph_id, a.second);
	}

	return plugins;
}

void
Serialiser::Impl::serialise_block(const SPtr<const Node>& block,
                                  const serd::Node&       class_id,
                                  const serd::Node&       block_id)
{
	const URIs& uris = _world.uris();

	_model->insert(block_id, uris.rdf_type, uris.ingen_Block);
	_model->insert(block_id, uris.lv2_prototype, class_id);

	// Serialise properties, but remove possibly stale state:state (set again below)
	Properties props = block->properties();
	props.erase(uris.state_state);
	serialise_properties(block_id, props);

	if (_base_uri.scheme() == "file") {
		const FilePath base_path  = _base_uri.file_path();
		const FilePath graph_dir  = base_path.parent_path();
		const FilePath state_dir  = graph_dir / block->symbol();
		const FilePath state_file = state_dir / "state.ttl";
		if (block->save_state(state_dir)) {
			_model->insert(block_id,
			               uris.state_state,
			               serd::make_uri((std::string)state_file));
		}
	}

	for (uint32_t i = 0; i < block->num_ports(); ++i) {
		Node* const      p       = block->port(i);
		const serd::Node port_id = path_rdf_node(p->path());
		serialise_port(p, Resource::Graph::DEFAULT, port_id);
		_model->insert(block_id, uris.lv2_port, port_id);
	}
}

void
Serialiser::Impl::serialise_port(const Node*       port,
                                 Resource::Graph   context,
                                 const serd::Node& port_id)
{
	URIs&      uris  = _world.uris();
	Properties props = port->properties(context);

	if (context == Resource::Graph::INTERNAL) {
		// Always write lv2:symbol for Graph ports (required for lv2:Plugin)
		_model->insert(port_id,
		               uris.lv2_symbol,
		               serd::make_string(port->path().symbol()));
	} else if (context == Resource::Graph::EXTERNAL) {
		// Never write lv2:index for plugin instances (not persistent/stable)
		props.erase(uris.lv2_index);
	}

	if (context == Resource::Graph::INTERNAL &&
	    port->has_property(uris.rdf_type, uris.lv2_ControlPort) &&
	    port->has_property(uris.rdf_type, uris.lv2_InputPort))
	{
		const Atom& val = port->get_property(uris.ingen_value);
		if (val.is_valid()) {
			props.erase(uris.lv2_default);
			props.emplace(uris.lv2_default, val);
		} else {
			_world.log().warn("Control input has no value, lv2:default omitted.\n");
		}
	} else if (context != Resource::Graph::INTERNAL &&
	           !port->has_property(uris.rdf_type, uris.lv2_InputPort)) {
		props.erase(uris.ingen_value);
	}

	serialise_properties(port_id, props);
}

void
Serialiser::serialise_arc(const serd::Optional<serd::Node>& parent,
                          const SPtr<const Arc>&            arc)
{
	return me->serialise_arc(parent, arc);
}

void
Serialiser::Impl::serialise_arc(const serd::Optional<serd::Node>& parent,
                                const SPtr<const Arc>&            arc)
{
	if (!_model) {
		throw std::logic_error(
			"serialise_arc called without serialisation in progress");
	}

	const URIs& uris = _world.uris();

	const serd::Node src    = path_rdf_node(arc->tail_path());
	const serd::Node dst    = path_rdf_node(arc->head_path());
	const serd::Node arc_id = _world.rdf_world().get_blank();
	_model->insert(arc_id, uris.ingen_tail, src);
	_model->insert(arc_id, uris.ingen_head, dst);

	if (parent) {
		_model->insert(*parent, uris.ingen_arc, arc_id);
	} else {
		_model->insert(arc_id, uris.rdf_type, uris.ingen_Arc);
	}
}

static bool
skip_property(ingen::URIs& uris, const serd::Node& predicate)
{
	return (predicate == INGEN__file ||
	        predicate == uris.ingen_arc ||
	        predicate == uris.ingen_block ||
	        predicate == uris.lv2_port);
}

void
Serialiser::Impl::serialise_properties(serd::Node        id,
                                       const Properties& props)
{
	serd::Env      env{_base_uri};
	serd::Inserter inserter{*_model, env, {}};

	for (const auto& p : props) {
		const serd::Node key = serd::make_uri(p.first.c_str());
		if (!skip_property(_world.uris(), key)) {
			if (p.second.type() == _world.uris().atom_URI &&
			    !strncmp(
			            (const char*)p.second.get_body(), "ingen:/main/", 13)) {
				/* Value is a graph URI relative to the running engine.
				   Chop the prefix and save the path relative to the graph file.
				   This allows saving references to bundle resources. */
				_streamer.write(inserter.sink(),
				                id,
				                key,
				                p.second.type(),
				                p.second.size(),
				                (const char*)p.second.get_body() + 13,
				                sratom::Flag::pretty_numbers);
			} else {
				_streamer.write(inserter.sink(),
				                id,
				                key,
				                *p.second.atom(),
				                sratom::Flag::pretty_numbers);
			}
		}
	}
}

} // namespace ingen
