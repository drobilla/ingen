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
#include "ingen/memory.hpp"
#include "ingen/runtime_paths.hpp"
#include "lv2/core/lv2.h"
#include "lv2/state/state.h"
#include "lv2/ui/ui.h"
#include "lv2/urid/urid.h"
#include "raul/Path.hpp"
#include "raul/Symbol.hpp"
#include "serd/serd.h"
#include "sord/sord.h"
#include "sord/sordmm.hpp"
#include "sratom/sratom.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

namespace ingen {

struct Serialiser::Impl {
	explicit Impl(World& world)
		: _root_path("/")
		, _mode(Mode::TO_FILE)
		, _world(world)
		, _model(nullptr)
		, _sratom(sratom_new(&_world.uri_map().urid_map()))
	{}

	~Impl() {
		sratom_free(_sratom);
	}

	Impl(const Impl&) = delete;
	Impl(Impl&&) = delete;
	Impl& operator=(const Impl&) = delete;
	Impl& operator=(Impl&&) = delete;

	enum class Mode { TO_FILE, TO_STRING };

	void start_to_file(const Raul::Path& root,
	                   const FilePath&   filename);

	std::set<const Resource*>
	serialise_graph(const std::shared_ptr<const Node>& graph,
	                const Sord::Node&                  graph_id);

	void serialise_block(const std::shared_ptr<const Node>& block,
	                     const Sord::Node&                  class_id,
	                     const Sord::Node&                  block_id);

	void serialise_port(const Node*       port,
	                    Resource::Graph   context,
	                    const Sord::Node& port_id);

	void serialise_properties(Sord::Node        id,
	                          const Properties& props);

	void write_bundle(const std::shared_ptr<const Node>& graph, const URI& uri);

	Sord::Node path_rdf_node(const Raul::Path& path) const;

	void write_manifest(const FilePath&                    bundle_path,
	                    const std::shared_ptr<const Node>& graph);

	void write_plugins(const FilePath&                  bundle_path,
	                   const std::set<const Resource*>& plugins);

	void serialise_arc(const Sord::Node&                 parent,
	                   const std::shared_ptr<const Arc>& arc);

	std::string finish();

	Raul::Path   _root_path;
	Mode         _mode;
	URI          _base_uri;
	FilePath     _basename;
	World&       _world;
	Sord::Model* _model;
	Sratom*      _sratom;
};

Serialiser::Serialiser(World& world)
	: me{make_unique<Impl>(world)}
{}

Serialiser::~Serialiser() = default;

void
Serialiser::Impl::write_manifest(const FilePath& bundle_path,
                                 const std::shared_ptr<const Node>&)
{
	const FilePath manifest_path(bundle_path / "manifest.ttl");
	const FilePath binary_path(ingen_module_path("lv2"));

	start_to_file(Raul::Path("/"), manifest_path);

	Sord::World& world = _model->world();
	const URIs&  uris  = _world.uris();

	const std::string filename("main.ttl");
	const Sord::URI   subject(world, filename, _base_uri);

	_model->add_statement(subject,
	                      Sord::URI(world, uris.rdf_type),
	                      Sord::URI(world, uris.ingen_Graph));
	_model->add_statement(subject,
	                      Sord::URI(world, uris.rdf_type),
	                      Sord::URI(world, uris.lv2_Plugin));
	_model->add_statement(subject,
	                      Sord::URI(world, uris.rdfs_seeAlso),
	                      Sord::URI(world, filename, _base_uri));
	_model->add_statement(subject,
	                      Sord::URI(world, uris.lv2_prototype),
	                      Sord::URI(world, uris.ingen_GraphPrototype));

	finish();
}

void
Serialiser::Impl::write_plugins(const FilePath&                  bundle_path,
                                const std::set<const Resource*>& plugins)
{
	const FilePath plugins_path(bundle_path / "plugins.ttl");

	start_to_file(Raul::Path("/"), plugins_path);

	Sord::World& world = _model->world();
	const URIs&  uris  = _world.uris();

	for (const auto& p : plugins) {
		const Atom& minor = p->get_property(uris.lv2_minorVersion);
		const Atom& micro = p->get_property(uris.lv2_microVersion);

		_model->add_statement(Sord::URI(world, p->uri()),
		                      Sord::URI(world, uris.rdf_type),
		                      Sord::URI(world, uris.lv2_Plugin));

		if (minor.is_valid() && micro.is_valid()) {
			_model->add_statement(Sord::URI(world, p->uri()),
			                      Sord::URI(world, uris.lv2_minorVersion),
			                      Sord::Literal::integer(world, minor.get<int32_t>()));
			_model->add_statement(Sord::URI(world, p->uri()),
			                      Sord::URI(world, uris.lv2_microVersion),
			                      Sord::Literal::integer(world, micro.get<int32_t>()));
		}
	}

	finish();
}

void
Serialiser::write_bundle(const std::shared_ptr<const Node>& graph,
                         const URI&                         uri)
{
	me->write_bundle(graph, uri);
}

void
Serialiser::Impl::write_bundle(const std::shared_ptr<const Node>& graph,
                               const URI&                         uri)
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
		Sord::URI(_model->world(), main_file, _base_uri));

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

	_model     = new Sord::Model(*_world.rdf_world(), _base_uri);
	_mode      = Mode::TO_FILE;
	_root_path = root;
}

void
Serialiser::start_to_string(const Raul::Path& root, const URI& base_uri)
{
	me->_root_path = root;
	me->_base_uri  = base_uri;
	me->_model     = new Sord::Model(*me->_world.rdf_world(), base_uri);
	me->_mode      = Impl::Mode::TO_STRING;
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
	std::string ret;
	if (_mode == Mode::TO_FILE) {
		SerdStatus st = _model->write_to_file(_base_uri, SERD_TURTLE);
		if (st) {
			_world.log().error("Error writing file %1% (%2%)\n",
			                   _base_uri, serd_strerror(st));
		}
	} else {
		ret = _model->write_to_string(_base_uri, SERD_TURTLE);
	}

	delete _model;
	_model    = nullptr;
	_base_uri = URI();

	return ret;
}

Sord::Node
Serialiser::Impl::path_rdf_node(const Raul::Path& path) const
{
	assert(_model);
	assert(path == _root_path || path.is_child_of(_root_path));
	return Sord::URI(_model->world(),
	                 path.substr(_root_path.base().length()),
	                 _base_uri);
}

void
Serialiser::serialise(const std::shared_ptr<const Node>& object,
                      Resource::Graph                    context)
{
	if (!me->_model) {
		throw std::logic_error("serialise called without serialisation in progress");
	}

	if (object->graph_type() == Node::GraphType::GRAPH) {
		me->serialise_graph(object, me->path_rdf_node(object->path()));
	} else if (object->graph_type() == Node::GraphType::BLOCK) {
		const Sord::URI plugin_id(me->_model->world(), object->plugin()->uri());
		me->serialise_block(object, plugin_id, me->path_rdf_node(object->path()));
	} else if (object->graph_type() == Node::GraphType::PORT) {
		me->serialise_port(
			object.get(), context, me->path_rdf_node(object->path()));
	} else {
		me->serialise_properties(me->path_rdf_node(object->path()),
		                         object->properties());
	}
}

std::set<const Resource*>
Serialiser::Impl::serialise_graph(const std::shared_ptr<const Node>& graph,
                                  const Sord::Node&                  graph_id)
{
	Sord::World& world = _model->world();
	const URIs&  uris  = _world.uris();

	_model->add_statement(graph_id,
	                      Sord::URI(world, uris.rdf_type),
	                      Sord::URI(world, uris.ingen_Graph));

	_model->add_statement(graph_id,
	                      Sord::URI(world, uris.rdf_type),
	                      Sord::URI(world, uris.lv2_Plugin));

	_model->add_statement(graph_id,
	                      Sord::URI(world, uris.lv2_extensionData),
	                      Sord::URI(world, LV2_STATE__interface));

	_model->add_statement(graph_id,
	                      Sord::URI(world, LV2_UI__ui),
	                      Sord::URI(world, "http://drobilla.net/ns/ingen#GraphUIGtk2"));

	// If the graph has no doap:name (required by LV2), use the basename
	if (graph->properties().find(uris.doap_name) == graph->properties().end()) {
		_model->add_statement(graph_id,
		                      Sord::URI(world, uris.doap_name),
		                      Sord::Literal(world, _basename));
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
			std::shared_ptr<Node> subgraph = n->second;

			SerdURI base_uri;
			serd_uri_parse(reinterpret_cast<const uint8_t*>(_base_uri.c_str()),
			               &base_uri);

			const std::string sub_bundle_path = subgraph->path().substr(1) + ".ingen";

			SerdURI  subgraph_uri;
			SerdNode subgraph_node = serd_node_new_uri_from_string(
				reinterpret_cast<const uint8_t*>(sub_bundle_path.c_str()),
				&base_uri,
				&subgraph_uri);

			const Sord::URI subgraph_id(world,
			                            reinterpret_cast<const char*>(
			                                subgraph_node.buf));

			// Save our state
			URI          my_base_uri = _base_uri;
			Sord::Model* my_model    = _model;

			// Write child bundle within this bundle
			write_bundle(subgraph, subgraph_id);

			// Restore our state
			_base_uri = my_base_uri;
			_model    = my_model;

			// Serialise reference to graph block
			const Sord::Node block_id(path_rdf_node(subgraph->path()));
			_model->add_statement(graph_id,
			                      Sord::URI(world, uris.ingen_block),
			                      block_id);
			serialise_block(subgraph, subgraph_id, block_id);

			serd_node_free(&subgraph_node);
		} else if (n->second->graph_type() == Node::GraphType::BLOCK) {
			std::shared_ptr<const Node> block = n->second;

			const Sord::URI  class_id(world, block->plugin()->uri());
			const Sord::Node block_id(path_rdf_node(n->second->path()));
			_model->add_statement(graph_id,
			                      Sord::URI(world, uris.ingen_block),
			                      block_id);
			serialise_block(block, class_id, block_id);

			plugins.insert(block->plugin());
		}
	}

	for (uint32_t i = 0; i < graph->num_ports(); ++i) {
		Node* p = graph->port(i);
		const Sord::Node port_id = path_rdf_node(p->path());

		// Ensure lv2:name always exists so Graph is a valid LV2 plugin
		if (p->properties().find(uris.lv2_name) == p->properties().end()) {
			p->set_property(uris.lv2_name,
			                _world.forge().alloc(p->symbol().c_str()));
		}

		_model->add_statement(graph_id,
		                      Sord::URI(world, LV2_CORE__port),
		                      port_id);
		serialise_port(p, Resource::Graph::DEFAULT, port_id);
		serialise_port(p, Resource::Graph::INTERNAL, port_id);
	}

	for (const auto& a : graph->arcs()) {
		serialise_arc(graph_id, a.second);
	}

	return plugins;
}

void
Serialiser::Impl::serialise_block(const std::shared_ptr<const Node>& block,
                                  const Sord::Node&                  class_id,
                                  const Sord::Node&                  block_id)
{
	const URIs& uris = _world.uris();

	_model->add_statement(block_id,
	                      Sord::URI(_model->world(), uris.rdf_type),
	                      Sord::URI(_model->world(), uris.ingen_Block));
	_model->add_statement(block_id,
	                      Sord::URI(_model->world(), uris.lv2_prototype),
	                      class_id);

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
			_model->add_statement(block_id,
			                      Sord::URI(_model->world(), uris.state_state),
			                      Sord::URI(_model->world(), URI(state_file)));
		}
	}

	for (uint32_t i = 0; i < block->num_ports(); ++i) {
		Node* const      p       = block->port(i);
		const Sord::Node port_id = path_rdf_node(p->path());
		serialise_port(p, Resource::Graph::DEFAULT, port_id);
		_model->add_statement(block_id,
		                      Sord::URI(_model->world(), uris.lv2_port),
		                      port_id);
	}
}

void
Serialiser::Impl::serialise_port(const Node*       port,
                                 Resource::Graph   context,
                                 const Sord::Node& port_id)
{
	URIs&            uris  = _world.uris();
	Sord::World&     world = _model->world();
	Properties props = port->properties(context);

	if (context == Resource::Graph::INTERNAL) {
		// Always write lv2:symbol for Graph ports (required for lv2:Plugin)
		_model->add_statement(port_id,
		                      Sord::URI(world, uris.lv2_symbol),
		                      Sord::Literal(world, port->path().symbol()));
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
Serialiser::serialise_arc(const Sord::Node&                 parent,
                          const std::shared_ptr<const Arc>& arc)
{
	return me->serialise_arc(parent, arc);
}

void
Serialiser::Impl::serialise_arc(const Sord::Node&                 parent,
                                const std::shared_ptr<const Arc>& arc)
{
	if (!_model) {
		throw std::logic_error(
			"serialise_arc called without serialisation in progress");
	}

	Sord::World& world = _model->world();
	const URIs&  uris  = _world.uris();

	const Sord::Node src    = path_rdf_node(arc->tail_path());
	const Sord::Node dst    = path_rdf_node(arc->head_path());
	const Sord::Node arc_id = Sord::Node::blank_id(*_world.rdf_world());
	_model->add_statement(arc_id,
	                      Sord::URI(world, uris.ingen_tail),
	                      src);
	_model->add_statement(arc_id,
	                      Sord::URI(world, uris.ingen_head),
	                      dst);

	if (parent.is_valid()) {
		_model->add_statement(parent,
		                      Sord::URI(world, uris.ingen_arc),
		                      arc_id);
	} else {
		_model->add_statement(arc_id,
		                      Sord::URI(world, uris.rdf_type),
		                      Sord::URI(world, uris.ingen_Arc));
	}
}

static bool
skip_property(ingen::URIs& uris, const Sord::Node& predicate)
{
	return (predicate == INGEN__file ||
	        predicate == uris.ingen_arc ||
	        predicate == uris.ingen_block ||
	        predicate == uris.lv2_port);
}

void
Serialiser::Impl::serialise_properties(Sord::Node        id,
                                       const Properties& props)
{
	LV2_URID_Unmap* unmap = &_world.uri_map().urid_unmap();
	SerdNode        base  = serd_node_from_string(SERD_URI,
                                          reinterpret_cast<const uint8_t*>(
                                              _base_uri.c_str()));

	SerdEnv*        env      = serd_env_new(&base);
	SordInserter*   inserter = sord_inserter_new(_model->c_obj(), env);

	sratom_set_sink(_sratom,
	                _base_uri.c_str(),
	                reinterpret_cast<SerdStatementSink>(
	                    sord_inserter_write_statement),
	                nullptr,
	                inserter);

	sratom_set_pretty_numbers(_sratom, true);

	for (const auto& p : props) {
		const Sord::URI key(_model->world(), p.first);
		if (!skip_property(_world.uris(), key)) {
			if (p.second.type() == _world.uris().atom_URI &&
			    !strncmp(reinterpret_cast<const char*>(p.second.get_body()),
			             "ingen:/main/",
			             12)) {
				/* Value is a graph URI relative to the running engine.
				   Chop the prefix and save the path relative to the graph file.
				   This allows saving references to bundle resources. */
				sratom_write(_sratom, unmap, 0,
				             sord_node_to_serd_node(id.c_obj()),
				             sord_node_to_serd_node(key.c_obj()),
				             p.second.type(), p.second.size(),
				             reinterpret_cast<const char*>(p.second.get_body()) + 13);
			} else {
				sratom_write(_sratom, unmap, 0,
				             sord_node_to_serd_node(id.c_obj()),
				             sord_node_to_serd_node(key.c_obj()),
				             p.second.type(), p.second.size(), p.second.get_body());
			}
		}
	}

	sord_inserter_free(inserter);
	serd_env_free(env);
}

} // namespace ingen
