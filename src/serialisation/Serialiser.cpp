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

#include <errno.h>
#include <string.h>

#include <cassert>
#include <cstdlib>
#include <string>
#include <utility>

#include <glib.h>
#include <glib/gstdio.h>
#include <glibmm/convert.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include <glibmm/module.h>

#include "ingen/Arc.hpp"
#include "ingen/Interface.hpp"
#include "ingen/Log.hpp"
#include "ingen/Node.hpp"
#include "ingen/Plugin.hpp"
#include "ingen/Resource.hpp"
#include "ingen/Store.hpp"
#include "ingen/URIMap.hpp"
#include "ingen/URIs.hpp"
#include "ingen/World.hpp"
#include "ingen/serialisation/Serialiser.hpp"
#include "lv2/lv2plug.in/ns/ext/state/state.h"
#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"
#include "raul/Path.hpp"
#include "sord/sordmm.hpp"
#include "sratom/sratom.h"

using namespace std;
using namespace Sord;
using namespace Ingen;

namespace Ingen {
namespace Serialisation {

struct Serialiser::Impl {
	typedef Resource::Properties Properties;

	explicit Impl(World& world)
		: _root_path("/")
		, _world(world)
		, _model(NULL)
		, _sratom(sratom_new(&_world.uri_map().urid_map_feature()->urid_map))
	{}

	~Impl() {
		sratom_free(_sratom);
	}

	enum class Mode { TO_FILE, TO_STRING };

	void start_to_filename(const std::string& filename);

	void serialise_graph(SPtr<const Node>  p,
	                     const Sord::Node& id);

	void serialise_block(SPtr<const Node>  n,
	                     const Sord::Node& class_id,
	                     const Sord::Node& id);

	void serialise_port(const Node*       p,
	                    Resource::Graph   context,
	                    const Sord::Node& id);

	void serialise_properties(Sord::Node                  id,
	                          const Resource::Properties& props);

	void write_bundle(SPtr<const Node>   graph,
	                  const std::string& uri);

	Sord::Node path_rdf_node(const Raul::Path& path);

	void write_manifest(const std::string& bundle_path,
	                    SPtr<const Node>   graph,
	                    const std::string& graph_symbol);

	void serialise_arc(const Sord::Node& parent,
	                   SPtr<const Arc>   a)
			throw (std::logic_error);

	std::string finish();

	Raul::Path   _root_path;
	Mode         _mode;
	std::string  _base_uri;
	World&       _world;
	Sord::Model* _model;
	Sratom*      _sratom;
};

Serialiser::Serialiser(World& world)
	: me(new Impl(world))
{}

Serialiser::~Serialiser()
{
	delete me;
}

void
Serialiser::Impl::write_manifest(const std::string& bundle_path,
                                 SPtr<const Node>   graph,
                                 const std::string& graph_symbol)
{
	const string manifest_path(Glib::build_filename(bundle_path, "manifest.ttl"));
	const string binary_path(Glib::Module::build_path("", "ingen_lv2"));

	start_to_filename(manifest_path);

	Sord::World& world = _model->world();
	const URIs&  uris  = _world.uris();

	const string    filename(graph_symbol + ".ttl");
	const Sord::URI subject(world, filename, _base_uri);

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
Serialiser::write_bundle(SPtr<const Node>   graph,
                         const std::string& path)
{
	me->write_bundle(graph, path);
}

void
Serialiser::Impl::write_bundle(SPtr<const Node>   graph,
                               const std::string& a_path)
{
	std::string path = Glib::filename_from_uri(a_path);
	if (Glib::file_test(path, Glib::FILE_TEST_EXISTS)
	    && !Glib::file_test(path, Glib::FILE_TEST_IS_DIR)) {
		path = Glib::path_get_dirname(path);
	}

	_world.log().info(fmt("Writing bundle %1%\n") % path);

	if (path[path.length() - 1] != '/')
		path.append("/");

	g_mkdir_with_parents(path.c_str(), 0744);

	string symbol = Glib::path_get_basename(path);
	symbol = symbol.substr(0, symbol.find("."));

	const string root_file = Glib::build_filename(path, symbol + ".ttl");

	start_to_filename(root_file);
	const Raul::Path old_root_path = _root_path;
	_root_path = graph->path();
	serialise_graph(graph, Sord::URI(_model->world(), root_file, _base_uri));
	_root_path = old_root_path;
	finish();

	write_manifest(path, graph, symbol);
}

/** Begin a serialization to a file.
 *
 * This must be called before any serializing methods.
 */
void
Serialiser::Impl::start_to_filename(const string& filename)
{
	assert(filename.find(":") == string::npos || filename.substr(0, 5) == "file:");
	if (filename.find(":") == string::npos) {
		_base_uri = "file://" + filename;
	} else {
		_base_uri = filename;
	}

	_model = new Sord::Model(*_world.rdf_world(), _base_uri);
	_mode = Mode::TO_FILE;
}

void
Serialiser::start_to_string(const Raul::Path& root, const string& base_uri)
{
	me->_root_path = root;
	me->_base_uri  = base_uri;
	me->_model     = new Sord::Model(*me->_world.rdf_world(), base_uri);
	me->_mode      = Impl::Mode::TO_STRING;
}

void
Serialiser::start_to_file(const Raul::Path& root, const string& filename)
{
	me->_root_path = root;
	me->start_to_filename(filename);
}

std::string
Serialiser::finish()
{
	return me->finish();
}

std::string
Serialiser::Impl::finish()
{
	string ret = "";
	if (_mode == Mode::TO_FILE) {
		SerdStatus st = _model->write_to_file(_base_uri, SERD_TURTLE);
		if (st) {
			_world.log().error(fmt("Error writing file %1% (%2%)\n")
			                   % _base_uri % serd_strerror(st));
		}
	} else {
		ret = _model->write_to_string(_base_uri, SERD_TURTLE);
	}

	delete _model;
	_model    = NULL;
	_base_uri = "";

	return ret;
}

Sord::Node
Serialiser::Impl::path_rdf_node(const Raul::Path& path)
{
	assert(_model);
	assert(path == _root_path || path.is_child_of(_root_path));
	return Sord::URI(_model->world(),
	                 path.substr(_root_path.base().length()),
	                 _base_uri);
}

void
Serialiser::serialise(SPtr<const Node> object) throw (std::logic_error)
{
	if (!me->_model)
		throw std::logic_error("serialise called without serialisation in progress");

	if (object->graph_type() == Node::GraphType::GRAPH) {
		me->serialise_graph(object, me->path_rdf_node(object->path()));
	} else if (object->graph_type() == Node::GraphType::BLOCK) {
		const Sord::URI plugin_id(me->_model->world(), object->plugin()->uri());
		me->serialise_block(object, plugin_id, me->path_rdf_node(object->path()));
	} else if (object->graph_type() == Node::GraphType::PORT) {
		me->serialise_port(object.get(),
		                   Resource::Graph::DEFAULT,
		                   me->path_rdf_node(object->path()));
	} else {
		me->serialise_properties(me->path_rdf_node(object->path()),
		                         object->properties());
	}
}

void
Serialiser::Impl::serialise_graph(SPtr<const Node>  graph,
                                  const Sord::Node& graph_id)
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

	// Always write a symbol (required by Ingen)
	Raul::Symbol symbol("_");
	Node::Properties::const_iterator s = graph->properties().find(uris.lv2_symbol);
	if (s == graph->properties().end()
	    || s->second.type() != _world.forge().String
	    || !Raul::Symbol::is_valid(s->second.ptr<char>())) {
		const std::string base = Glib::path_get_basename(
			_model->base_uri().to_c_string());
		symbol = Raul::Symbol::symbolify(base.substr(0, base.find('.')));
		_model->add_statement(
			graph_id,
			Sord::URI(world, uris.lv2_symbol),
			Sord::Literal(world, symbol.c_str()));
	} else {
		symbol = Raul::Symbol::symbolify(s->second.ptr<char>());
	}

	// If the graph has no doap:name (required by LV2), use the symbol
	if (graph->properties().find(uris.doap_name) == graph->properties().end())
		_model->add_statement(graph_id,
		                      Sord::URI(world, uris.doap_name),
		                      Sord::Literal(world, symbol.c_str()));

	const Node::Properties props = graph->properties(Resource::Graph::INTERNAL);
	serialise_properties(graph_id, props);

	const Store::const_range kids = _world.store()->children_range(graph);
	for (Store::const_iterator n = kids.first; n != kids.second; ++n) {
		if (n->first.parent() != graph->path())
			continue;

		if (n->second->graph_type() == Node::GraphType::GRAPH) {
			SPtr<Node> subgraph = n->second;

			SerdURI base_uri;
			serd_uri_parse((const uint8_t*)_base_uri.c_str(), &base_uri);

			const string sub_bundle_path = subgraph->path().substr(1) + ".ingen";

			SerdURI  subgraph_uri;
			SerdNode subgraph_node = serd_node_new_uri_from_string(
				(const uint8_t*)sub_bundle_path.c_str(),
				&base_uri,
				&subgraph_uri);

			const Sord::URI subgraph_id(world, (const char*)subgraph_node.buf);

			// Save our state
			std::string  my_base_uri = _base_uri;
			Sord::Model* my_model    = _model;

			// Write child bundle within this bundle
			write_bundle(subgraph, subgraph_id.to_string());

			// Restore our state
			_base_uri = my_base_uri;
			_model    = my_model;

			// Serialise reference to graph block
			const Sord::Node block_id(path_rdf_node(subgraph->path()));
			_model->add_statement(graph_id,
			                      Sord::URI(world, uris.ingen_block),
			                      block_id);
			serialise_block(subgraph, subgraph_id, block_id);
		} else if (n->second->graph_type() == Node::GraphType::BLOCK) {
			SPtr<const Node> block = n->second;

			const Sord::URI  class_id(world, block->plugin()->uri());
			const Sord::Node block_id(path_rdf_node(n->second->path()));
			_model->add_statement(graph_id,
			                      Sord::URI(world, uris.ingen_block),
			                      block_id);
			serialise_block(block, class_id, block_id);
		}
	}

	for (uint32_t i = 0; i < graph->num_ports(); ++i) {
		Node* p = graph->port(i);
		const Sord::Node port_id = path_rdf_node(p->path());

		// Ensure lv2:name always exists so Graph is a valid LV2 plugin
		if (p->properties().find(uris.lv2_name) == p->properties().end())
			p->set_property(uris.lv2_name,
			                _world.forge().alloc(p->symbol().c_str()));

		_model->add_statement(graph_id,
		                      Sord::URI(world, LV2_CORE__port),
		                      port_id);
		serialise_port(p, Resource::Graph::INTERNAL, port_id);
	}

	for (const auto& a : graph->arcs()) {
		serialise_arc(graph_id, a.second);
	}
}

void
Serialiser::Impl::serialise_block(SPtr<const Node>  block,
                                  const Sord::Node& class_id,
                                  const Sord::Node& block_id)
{
	const URIs& uris = _world.uris();

	_model->add_statement(block_id,
	                      Sord::URI(_model->world(), uris.rdf_type),
	                      Sord::URI(_model->world(), uris.ingen_Block));
	_model->add_statement(block_id,
	                      Sord::URI(_model->world(), uris.ingen_prototype),
	                      class_id);

	const Node::Properties props = block->properties(Resource::Graph::EXTERNAL);
	serialise_properties(block_id, props);

	for (uint32_t i = 0; i < block->num_ports(); ++i) {
		Node* const      p       = block->port(i);
		const Sord::Node port_id = path_rdf_node(p->path());
		serialise_port(p, Resource::Graph::EXTERNAL, port_id);
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
	Node::Properties props = port->properties(context);

	if (context == Resource::Graph::INTERNAL) {
		// Always write lv2:symbol for Graph ports (required for lv2:Plugin)
		_model->add_statement(port_id,
		                      Sord::URI(world, uris.lv2_symbol),
		                      Sord::Literal(world, port->path().symbol()));
	} else {
		// Never write lv2:index for plugin instances (not persistent/stable)
		props.erase(uris.lv2_index);
	}

	if (context == Resource::Graph::INTERNAL &&
	    port->has_property(uris.rdf_type, uris.lv2_ControlPort) &&
	    port->has_property(uris.rdf_type, uris.lv2_InputPort))
	{
		const Atom& val = port->get_property(uris.ingen_value);
		if (val.is_valid()) {
			props.insert(make_pair(uris.lv2_default, val));
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
Serialiser::serialise_arc(const Sord::Node& parent,
                          SPtr<const Arc>   arc)
		throw (std::logic_error)
{
	return me->serialise_arc(parent, arc);
}

void
Serialiser::Impl::serialise_arc(const Sord::Node& parent,
                                SPtr<const Arc>   arc)
		throw (std::logic_error)
{
	if (!_model)
		throw std::logic_error(
			"serialise_arc called without serialisation in progress");

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
skip_property(Ingen::URIs& uris, const Sord::Node& predicate)
{
	return (predicate.to_string() == INGEN__file ||
	        predicate.to_string() == uris.ingen_arc ||
	        predicate.to_string() == uris.ingen_block ||
	        predicate.to_string() == uris.lv2_port);
}

void
Serialiser::Impl::serialise_properties(Sord::Node              id,
                                       const Node::Properties& props)
{
	LV2_URID_Unmap* unmap    = &_world.uri_map().urid_unmap_feature()->urid_unmap;
	SerdNode        base     = serd_node_from_string(SERD_URI,
	                                                 (const uint8_t*)_base_uri.c_str());
	SerdEnv*        env      = serd_env_new(&base);
	SordInserter*   inserter = sord_inserter_new(_model->c_obj(), env);

	sratom_set_sink(_sratom, _base_uri.c_str(),
	                (SerdStatementSink)sord_inserter_write_statement, NULL,
	                inserter);

	sratom_set_pretty_numbers(_sratom, true);

	for (const auto& p : props) {
		const Sord::URI key(_model->world(), p.first);
		if (!skip_property(_world.uris(), key)) {
			sratom_write(_sratom, unmap, 0,
			             sord_node_to_serd_node(id.c_obj()),
			             sord_node_to_serd_node(key.c_obj()),
			             p.second.type(), p.second.size(), p.second.get_body());
		}
	}

	sord_inserter_free(inserter);
	serd_env_free(env);
}

} // namespace Serialisation
} // namespace Ingen
