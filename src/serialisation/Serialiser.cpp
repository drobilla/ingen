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

#include "ingen/Edge.hpp"
#include "ingen/GraphObject.hpp"
#include "ingen/Interface.hpp"
#include "ingen/Log.hpp"
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
	explicit Impl(World& world)
		: _root_path("/")
		, _world(world)
		, _model(NULL)
		, _sratom(sratom_new(&_world.uri_map().urid_map_feature()->urid_map))
	{}

	~Impl() {
		sratom_free(_sratom);
	}

	enum Mode { TO_FILE, TO_STRING };

	void start_to_filename(const std::string& filename);

	void serialise_graph(SharedPtr<const GraphObject> p,
	                     const Sord::Node&            id);

	void serialise_block(SharedPtr<const GraphObject> n,
	                     const Sord::Node&            class_id,
	                     const Sord::Node&            id);

	void serialise_port(const GraphObject* p,
	                    Resource::Graph    context,
	                    const Sord::Node&  id);

	void serialise_properties(Sord::Node                  id,
	                          const Resource::Properties& props);

	void write_bundle(SharedPtr<const GraphObject> graph,
	                  const std::string&           uri);

	Sord::Node path_rdf_node(const Raul::Path& path);

	void write_manifest(const std::string&           bundle_path,
	                    SharedPtr<const GraphObject> graph,
	                    const std::string&           graph_symbol);

	void serialise_edge(const Sord::Node& parent,
	                    SharedPtr<const Edge> c)
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
Serialiser::to_file(SharedPtr<const GraphObject> object,
                    const std::string&           filename)
{
	me->_root_path = object->path();
	me->start_to_filename(filename);
	serialise(object);
	finish();
}

void
Serialiser::Impl::write_manifest(const std::string&           bundle_path,
                                 SharedPtr<const GraphObject> graph,
                                 const std::string&           graph_symbol)
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
	                      Sord::URI(world, uris.lv2_binary),
	                      Sord::URI(world, binary_path, _base_uri));

	symlink(Glib::Module::build_path(INGEN_BUNDLE_DIR, "ingen_lv2").c_str(),
	        Glib::Module::build_path(bundle_path, "ingen_lv2").c_str());

	finish();
}

void
Serialiser::write_bundle(SharedPtr<const GraphObject> graph,
                         const std::string&           path)
{
	me->write_bundle(graph, path);
}

void
Serialiser::Impl::write_bundle(SharedPtr<const GraphObject> graph,
                               const std::string&           a_path)
{
	std::string path = Glib::filename_from_uri(a_path);
	if (Glib::file_test(path, Glib::FILE_TEST_EXISTS)
	    && !Glib::file_test(path, Glib::FILE_TEST_IS_DIR)) {
		path = Glib::path_get_dirname(path);
	}

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

string
Serialiser::to_string(SharedPtr<const GraphObject> object,
                      const string&                base_uri)
{
	start_to_string(object->path(), base_uri);
	serialise(object);
	return finish();
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
	_mode = TO_FILE;
}

/** Begin a serialization to a string.
 *
 * This must be called before any serializing methods.
 *
 * The results of the serialization will be returned by the finish() method after
 * the desired objects have been serialised.
 *
 * All serialized paths will have the root path chopped from their prefix
 * (therefore all serialized paths must be descendants of the root)
 */
void
Serialiser::start_to_string(const Raul::Path& root, const string& base_uri)
{
	me->_root_path = root;
	me->_base_uri  = base_uri;
	me->_model     = new Sord::Model(*me->_world.rdf_world(), base_uri);
	me->_mode      = Impl::TO_STRING;
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
	if (_mode == TO_FILE) {
		SerdStatus st = _model->write_to_file(_base_uri, SERD_TURTLE);
		if (st) {
			_world.log().error(Raul::fmt("Error writing file %1% (%2%)\n")
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
	// FIXME: if path == root_path() then "/" ?
	return Sord::URI(_model->world(),
	                 path.substr(_root_path.base().length()),
	                 _base_uri);
}

void
Serialiser::serialise(SharedPtr<const GraphObject> object) throw (std::logic_error)
{
	if (!me->_model)
		throw std::logic_error("serialise called without serialisation in progress");

	if (object->graph_type() == GraphObject::GRAPH) {
		me->serialise_graph(object, me->path_rdf_node(object->path()));
	} else if (object->graph_type() == GraphObject::BLOCK) {
		const Sord::URI plugin_id(me->_model->world(), object->plugin()->uri());
		me->serialise_block(object, plugin_id, me->path_rdf_node(object->path()));
	} else if (object->graph_type() == GraphObject::PORT) {
		me->serialise_port(object.get(),
		                   Resource::DEFAULT,
		                   me->path_rdf_node(object->path()));
	} else {
		me->serialise_properties(me->path_rdf_node(object->path()),
		                         object->properties());
	}
}

void
Serialiser::Impl::serialise_graph(SharedPtr<const GraphObject> graph,
                                  const Sord::Node&            graph_id)
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
	GraphObject::Properties::const_iterator s = graph->properties().find(uris.lv2_symbol);
	if (s == graph->properties().end()
	    || !s->second.type() == _world.forge().String
	    || !Raul::Symbol::is_valid(s->second.get_string())) {
		const std::string base = Glib::path_get_basename(
			_model->base_uri().to_c_string());
		symbol = Raul::Symbol::symbolify(base.substr(0, base.find('.')));
		_model->add_statement(
			graph_id,
			Sord::URI(world, uris.lv2_symbol),
			Sord::Literal(world, symbol.c_str()));
	} else {
		symbol = Raul::Symbol::symbolify(s->second.get_string());
	}

	// If the graph has no doap:name (required by LV2), use the symbol
	if (graph->properties().find(uris.doap_name) == graph->properties().end())
		_model->add_statement(graph_id,
		                      Sord::URI(world, uris.doap_name),
		                      Sord::Literal(world, symbol.c_str()));

	const GraphObject::Properties props = graph->properties(Resource::INTERNAL);
	serialise_properties(graph_id, props);

	const Store::const_range kids = _world.store()->children_range(graph);
	for (Store::const_iterator n = kids.first; n != kids.second; ++n) {
		if (n->first.parent() != graph->path())
			continue;

		if (n->second->graph_type() == GraphObject::GRAPH) {
			SharedPtr<GraphObject> subgraph = n->second;

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
		} else if (n->second->graph_type() == GraphObject::BLOCK) {
			SharedPtr<const GraphObject> block = n->second;

			const Sord::URI  class_id(world, block->plugin()->uri());
			const Sord::Node block_id(path_rdf_node(n->second->path()));
			_model->add_statement(graph_id,
			                      Sord::URI(world, uris.ingen_block),
			                      block_id);
			serialise_block(block, class_id, block_id);
		}
	}

	for (uint32_t i = 0; i < graph->num_ports(); ++i) {
		GraphObject* p = graph->port(i);
		const Sord::Node port_id = path_rdf_node(p->path());

		// Ensure lv2:name always exists so Graph is a valid LV2 plugin
		if (p->properties().find(uris.lv2_name) == p->properties().end())
			p->set_property(uris.lv2_name,
			                _world.forge().alloc(p->symbol().c_str()));

		_model->add_statement(graph_id,
		                      Sord::URI(world, LV2_CORE__port),
		                      port_id);
		serialise_port(p, Resource::INTERNAL, port_id);
	}

	for (GraphObject::Edges::const_iterator c = graph->edges().begin();
	     c != graph->edges().end(); ++c) {
		serialise_edge(graph_id, c->second);
	}
}

void
Serialiser::Impl::serialise_block(SharedPtr<const GraphObject> block,
                                  const Sord::Node&            class_id,
                                  const Sord::Node&            block_id)
{
	const URIs& uris = _world.uris();

	_model->add_statement(block_id,
	                      Sord::URI(_model->world(), uris.rdf_type),
	                      Sord::URI(_model->world(), uris.ingen_Block));
	_model->add_statement(block_id,
	                      Sord::URI(_model->world(), uris.ingen_prototype),
	                      class_id);
	_model->add_statement(block_id,
	                      Sord::URI(_model->world(), uris.lv2_symbol),
	                      Sord::Literal(_model->world(), block->path().symbol()));

	const GraphObject::Properties props = block->properties(Resource::EXTERNAL);
	serialise_properties(block_id, props);

	for (uint32_t i = 0; i < block->num_ports(); ++i) {
		GraphObject* const p       = block->port(i);
		const Sord::Node   port_id = path_rdf_node(p->path());
		serialise_port(p, Resource::EXTERNAL, port_id);
		_model->add_statement(block_id,
		                      Sord::URI(_model->world(), uris.lv2_port),
		                      port_id);
	}
}

void
Serialiser::Impl::serialise_port(const GraphObject* port,
                                 Resource::Graph    context,
                                 const Sord::Node&  port_id)
{
	URIs&        uris  = _world.uris();
	Sord::World& world = _model->world();

	_model->add_statement(port_id,
	                      Sord::URI(world, uris.lv2_symbol),
	                      Sord::Literal(world, port->path().symbol()));

	GraphObject::Properties props = port->properties(context);
	if (context == Resource::INTERNAL &&
	    port->has_property(uris.rdf_type, uris.lv2_ControlPort) &&
	    port->has_property(uris.rdf_type, uris.lv2_InputPort))
	{
		const Raul::Atom& val = port->get_property(uris.ingen_value);
		if (val.is_valid()) {
			props.insert(make_pair(uris.lv2_default, val));
		} else {
			_world.log().warn("Control input has no value, lv2:default omitted.\n");
		}
	}

	serialise_properties(port_id, props);
}

void
Serialiser::serialise_edge(const Sord::Node&     parent,
                           SharedPtr<const Edge> edge)
		throw (std::logic_error)
{
	return me->serialise_edge(parent, edge);
}

void
Serialiser::Impl::serialise_edge(const Sord::Node&     parent,
                                 SharedPtr<const Edge> edge)
		throw (std::logic_error)
{
	if (!_model)
		throw std::logic_error(
			"serialise_edge called without serialisation in progress");

	Sord::World& world = _model->world();
	const URIs&  uris  = _world.uris();

	const Sord::Node src     = path_rdf_node(edge->tail_path());
	const Sord::Node dst     = path_rdf_node(edge->head_path());
	const Sord::Node edge_id = Sord::Node::blank_id(*_world.rdf_world());
	_model->add_statement(edge_id,
	                      Sord::URI(world, uris.ingen_tail),
	                      src);
	_model->add_statement(edge_id,
	                      Sord::URI(world, uris.ingen_head),
	                      dst);

	if (parent.is_valid()) {
		_model->add_statement(parent,
		                      Sord::URI(world, uris.ingen_edge),
		                      edge_id);
	} else {
		_model->add_statement(edge_id,
		                      Sord::URI(world, uris.rdf_type),
		                      Sord::URI(world, uris.ingen_Edge));
	}
}

static bool
skip_property(const Sord::Node& predicate)
{
	return (predicate.to_string() == "http://drobilla.net/ns/ingen#document");
}

void
Serialiser::Impl::serialise_properties(Sord::Node                     id,
                                       const GraphObject::Properties& props)
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

	typedef GraphObject::Properties::const_iterator iterator;
	for (iterator v = props.begin(); v != props.end(); ++v) {
		const Sord::URI key(_model->world(), v->first);
		if (!skip_property(key)) {
			sratom_write(_sratom, unmap, 0,
			             sord_node_to_serd_node(id.c_obj()),
			             sord_node_to_serd_node(key.c_obj()),
			             v->second.type(), v->second.size(), v->second.get_body());
		}
	}

	sord_inserter_free(inserter);
	serd_env_free(env);
}

} // namespace Serialisation
} // namespace Ingen
