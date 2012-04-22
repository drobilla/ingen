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

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>

#include <glib.h>
#include <glib/gstdio.h>
#include <glibmm/convert.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include <glibmm/module.h>

#include "ingen/Connection.hpp"
#include "ingen/Interface.hpp"
#include "ingen/Node.hpp"
#include "ingen/Patch.hpp"
#include "ingen/Plugin.hpp"
#include "ingen/Port.hpp"
#include "ingen/serialisation/Serialiser.hpp"
#include "ingen/shared/LV2URIMap.hpp"
#include "ingen/shared/ResourceImpl.hpp"
#include "ingen/shared/Store.hpp"
#include "ingen/shared/URIs.hpp"
#include "ingen/shared/World.hpp"
#include "raul/Atom.hpp"
#include "raul/Path.hpp"
#include "raul/log.hpp"
#include "sord/sordmm.hpp"
#include "sratom/sratom.h"

#define LOG(s) s << "[Serialiser] "

using namespace std;
using namespace Raul;
using namespace Sord;
using namespace Ingen;
using namespace Ingen::Shared;

namespace Ingen {
namespace Serialisation {

struct Serialiser::Impl {
	Impl(Shared::World& world)
		: _root_path("/")
		, _world(world)
	{}

	enum Mode { TO_FILE, TO_STRING };

	void start_to_filename(const std::string& filename);

	void serialise_patch(SharedPtr<const Patch> p,
	                     const Sord::Node&      id);

	void serialise_node(SharedPtr<const Node> n,
	                    const Sord::Node&     class_id,
	                    const Sord::Node&     id);

	void serialise_port(const Port*       p,
	                    Resource::Graph   context,
	                    const Sord::Node& id);

	void serialise_properties(const GraphObject* o,
	                          Resource::Graph    context,
	                          Sord::Node         id);

	void write_bundle(SharedPtr<const Patch> patch,
	                  const std::string&     uri);

	Sord::Node path_rdf_node(const Raul::Path& path);

	void write_manifest(const std::string&     bundle_path,
	                    SharedPtr<const Patch> patch,
	                    const std::string&     patch_symbol);

	void serialise_connection(const Sord::Node& parent,
	                          SharedPtr<const Connection> c)
			throw (std::logic_error);

	std::string finish();

	Raul::Path               _root_path;
	SharedPtr<Shared::Store> _store;
	Mode                     _mode;
	std::string              _base_uri;
	Shared::World&           _world;
	Sord::Model*             _model;
};

Serialiser::Serialiser(Shared::World& world)
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
Serialiser::Impl::write_manifest(const std::string&     bundle_path,
                                 SharedPtr<const Patch> patch,
                                 const std::string&     patch_symbol)
{
	const string manifest_path(Glib::build_filename(bundle_path, "manifest.ttl"));
	const string binary_path(Glib::Module::build_path("", "ingen_lv2"));

	start_to_filename(manifest_path);

	Sord::World& world = _model->world();

	const string    filename(patch_symbol + ".ttl");
	const Sord::URI subject(world, filename, _base_uri);

	_model->add_statement(subject,
	                      Sord::Curie(world, "rdf:type"),
	                      Sord::Curie(world, "ingen:Patch"));
	_model->add_statement(subject,
	                      Sord::Curie(world, "rdf:type"),
	                      Sord::Curie(world, "lv2:Plugin"));
	_model->add_statement(subject,
	                      Sord::Curie(world, "rdfs:seeAlso"),
	                      Sord::URI(world, filename, _base_uri));
	_model->add_statement(subject,
	                      Sord::Curie(world, "lv2:binary"),
	                      Sord::URI(world, binary_path, _base_uri));

	symlink(Glib::Module::build_path(INGEN_BUNDLE_DIR, "ingen_lv2").c_str(),
	        Glib::Module::build_path(bundle_path, "ingen_lv2").c_str());

	finish();
}

std::string
normal_bundle_uri(const std::string& uri)
{
	std::string ret = uri;
	size_t i;
	while ((i = ret.find("/./")) != std::string::npos) {
		ret = ret.substr(0, i) + ret.substr(i + 2);
	}
	const size_t last_slash = ret.find_last_of("/");
	if (last_slash != std::string::npos) {
		return ret.substr(0, last_slash);
	} else {
		return ret + "/";
	}
	return ret;
}

void
Serialiser::write_bundle(SharedPtr<const Patch> patch,
                         const std::string&     path)
{
	me->write_bundle(patch, path);
}

void
Serialiser::Impl::write_bundle(SharedPtr<const Patch> patch,
                               const std::string&     a_path)
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
	const Path old_root_path = _root_path;
	_root_path = patch->path();
	serialise_patch(patch, Sord::URI(_model->world(), root_file, _base_uri));
	_root_path = old_root_path;
	finish();

	write_manifest(path, patch, symbol);
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
			LOG(error) << "Error writing file `" << _base_uri << "' ("
			           << serd_strerror(st) << ")" << std::endl;
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
Serialiser::Impl::path_rdf_node(const Path& path)
{
	assert(_model);
	assert(path == _root_path || path.is_child_of(_root_path));
	const Path rel_path(path.relative_to_base(_root_path));
	return Sord::URI(_model->world(),
	                 rel_path.chop_scheme().substr(1).c_str(),
	                 _base_uri);
}

void
Serialiser::serialise(SharedPtr<const GraphObject> object) throw (std::logic_error)
{
	if (!me->_model)
		throw std::logic_error("serialise called without serialisation in progress");

	SharedPtr<const Patch> patch = PtrCast<const Patch>(object);
	if (patch) {
		me->serialise_patch(patch, me->path_rdf_node(patch->path()));
		return;
	}

	SharedPtr<const Node> node = PtrCast<const Node>(object);
	if (node) {
		const Sord::URI plugin_id(me->_model->world(), node->plugin()->uri().str());
		me->serialise_node(node, plugin_id, me->path_rdf_node(node->path()));
		return;
	}

	SharedPtr<const Port> port = PtrCast<const Port>(object);
	if (port) {
		me->serialise_port(port.get(), Resource::DEFAULT, me->path_rdf_node(port->path()));
		return;
	}

	LOG(warn) << "Unsupported object type, "
	          << object->path() << " not serialised." << endl;
}

void
Serialiser::Impl::serialise_patch(SharedPtr<const Patch> patch,
                                  const Sord::Node&      patch_id)
{
	assert(_model);
	Sord::World& world = _model->world();

	_model->add_statement(patch_id,
	                      Sord::Curie(world, "rdf:type"),
	                      Sord::Curie(world, "ingen:Patch"));

	_model->add_statement(patch_id,
	                      Sord::Curie(world, "rdf:type"),
	                      Sord::Curie(world, "lv2:Plugin"));

	_model->add_statement(patch_id,
	                      Sord::Curie(world, "lv2:extensionData"),
	                      Sord::URI(world, "http://lv2plug.in/ns/ext/state#Interface"));

	_model->add_statement(patch_id,
	                      Sord::URI(world, "http://lv2plug.in/ns/extensions/ui#ui"),
	                      Sord::URI(world, "http://drobilla.net/ns/ingen#ui"));

	const URIs& uris = *_world.uris().get();

	// Always write a symbol (required by Ingen)
	string symbol;
	GraphObject::Properties::const_iterator s = patch->properties().find(uris.lv2_symbol);
	if (s == patch->properties().end()
	    || !s->second.type() == _world.forge().String
	    || !Symbol::is_valid(s->second.get_string())) {
		symbol = Glib::path_get_basename(_model->base_uri().to_c_string());
		symbol = Symbol::symbolify(symbol.substr(0, symbol.find('.')));
		_model->add_statement(
			patch_id,
			Sord::Curie(world, "lv2:symbol"),
			Sord::Literal(world, symbol));
	} else {
		symbol = s->second.get_string();
	}

	// If the patch has no doap:name (required by LV2), use the symbol
	if (patch->properties().find(uris.doap_name) == patch->properties().end())
		_model->add_statement(patch_id,
		                      Sord::URI(world, uris.doap_name.str()),
		                      Sord::Literal(world, symbol));

	serialise_properties(patch.get(), Resource::INTERNAL, patch_id);

	for (Store::const_iterator n = _world.store()->children_begin(patch);
	     n != _world.store()->children_end(patch); ++n) {

		if (n->first.parent() != patch->path())
			continue;

		SharedPtr<Patch> subpatch = PtrCast<Patch>(n->second);
		SharedPtr<Node>  node     = PtrCast<Node>(n->second);
		if (subpatch) {
			SerdURI base_uri;
			serd_uri_parse((const uint8_t*)_base_uri.c_str(), &base_uri);

			const string sub_bundle_path = subpatch->path().chop_start("/") + ".ingen";

			SerdURI  subpatch_uri;
			SerdNode subpatch_node = serd_node_new_uri_from_string(
				(const uint8_t*)sub_bundle_path.c_str(),
				&base_uri,
				&subpatch_uri);

			const Sord::URI subpatch_id(world, (const char*)subpatch_node.buf);

			// Save our state
			std::string  my_base_uri = _base_uri;
			Sord::Model* my_model    = _model;

			// Write child bundle within this bundle
			write_bundle(subpatch, subpatch_id.to_string());

			// Restore our state
			_base_uri = my_base_uri;
			_model    = my_model;

			// Serialise reference to patch node
			const Sord::Node node_id(path_rdf_node(subpatch->path()));
			_model->add_statement(patch_id,
			                      Sord::Curie(world, "ingen:node"),
			                      node_id);
			serialise_node(subpatch, subpatch_id, node_id);
		} else if (node) {
			const Sord::URI  class_id(world, node->plugin()->uri().str());
			const Sord::Node node_id(path_rdf_node(n->second->path()));
			_model->add_statement(patch_id,
			                      Sord::Curie(world, "ingen:node"),
			                      node_id);
			serialise_node(node, class_id, node_id);
		}
	}

	for (uint32_t i = 0; i < patch->num_ports(); ++i) {
		Port* p = patch->port(i);
		const Sord::Node port_id = path_rdf_node(p->path());

		// Ensure lv2:name always exists so Patch is a valid LV2 plugin
		if (p->properties().find(LV2_CORE__name) == p->properties().end())
			p->set_property(LV2_CORE__name,
			                _world.forge().alloc(p->symbol().c_str()));

		_model->add_statement(patch_id,
		                      Sord::URI(world, LV2_CORE__port),
		                      port_id);
		serialise_port(p, Resource::INTERNAL, port_id);
	}

	for (Patch::Connections::const_iterator c = patch->connections().begin();
	     c != patch->connections().end(); ++c) {
		serialise_connection(patch_id, c->second);
	}
}

void
Serialiser::Impl::serialise_node(SharedPtr<const Node> node,
                                 const Sord::Node&     class_id,
                                 const Sord::Node&     node_id)
{
	_model->add_statement(node_id,
	                      Sord::Curie(_model->world(), "rdf:type"),
	                      Sord::Curie(_model->world(), "ingen:Node"));
	_model->add_statement(node_id,
	                      Sord::Curie(_model->world(), "rdf:instanceOf"),
	                      class_id);
	_model->add_statement(node_id,
	                      Sord::Curie(_model->world(), "lv2:symbol"),
	                      Sord::Literal(_model->world(), node->path().symbol()));

	serialise_properties(node.get(), Resource::EXTERNAL, node_id);

	for (uint32_t i = 0; i < node->num_ports(); ++i) {
		Port* const      p       = node->port(i);
		const Sord::Node port_id = path_rdf_node(p->path());
		serialise_port(p, Resource::EXTERNAL, port_id);
		_model->add_statement(node_id,
		                      Sord::Curie(_model->world(), "lv2:port"),
		                      port_id);
	}
}

void
Serialiser::Impl::serialise_port(const Port*       port,
                                 Resource::Graph   context,
                                 const Sord::Node& port_id)
{
	Sord::World& world = _model->world();

	if (port->is_input()) {
		_model->add_statement(port_id,
		                      Sord::Curie(world, "rdf:type"),
		                      Sord::Curie(world, "lv2:InputPort"));
	} else {
		_model->add_statement(port_id,
		                      Sord::Curie(world, "rdf:type"),
		                      Sord::Curie(world, "lv2:OutputPort"));
	}

	_model->add_statement(port_id,
	                      Sord::Curie(world, "lv2:symbol"),
	                      Sord::Literal(world, port->path().symbol()));

	serialise_properties(port, context, port_id);
}

void
Serialiser::serialise_connection(const Sord::Node&           parent,
                                 SharedPtr<const Connection> connection)
		throw (std::logic_error)
{
	return me->serialise_connection(parent, connection);
}

void
Serialiser::Impl::serialise_connection(const Sord::Node&           parent,
                                       SharedPtr<const Connection> connection)
		throw (std::logic_error)
{
	if (!_model)
		throw std::logic_error(
			"serialise_connection called without serialisation in progress");

	Sord::World& world = _model->world();

	const Sord::Node src           = path_rdf_node(connection->src_port_path());
	const Sord::Node dst           = path_rdf_node(connection->dst_port_path());
	const Sord::Node connection_id = Sord::Node::blank_id(*_world.rdf_world());
	_model->add_statement(connection_id,
	                      Sord::Curie(world, "ingen:source"),
	                      src);
	_model->add_statement(connection_id,
	                      Sord::Curie(world, "ingen:destination"),
	                      dst);

	_model->add_statement(parent,
	                      Sord::Curie(world, "ingen:connection"),
	                      connection_id);
}

static bool
skip_property(const Sord::Node& predicate)
{
	return (predicate.to_string() == "http://drobilla.net/ns/ingen#document");
}

void
Serialiser::Impl::serialise_properties(const GraphObject*     o,
                                       Ingen::Resource::Graph context,
                                       Sord::Node             id)
{
	const GraphObject::Properties props = o->properties(context);

	LV2_URID_Map*   map      = &_world.lv2_uri_map()->urid_map_feature()->urid_map;
	LV2_URID_Unmap* unmap    = &_world.lv2_uri_map()->urid_unmap_feature()->urid_unmap;
	Sratom*         sratom   = sratom_new(map);
	SerdNode        base     = serd_node_from_string(SERD_URI,
	                                                 (const uint8_t*)_base_uri.c_str());
	SerdEnv*        env      = serd_env_new(&base);
	SordInserter*   inserter = sord_inserter_new(_model->c_obj(), env);

	sratom_set_sink(sratom, _base_uri.c_str(),
	                (SerdStatementSink)sord_inserter_write_statement, NULL,
	                inserter);

	typedef GraphObject::Properties::const_iterator iterator;
	for (iterator v = props.begin(); v != props.end(); ++v) {
		const Sord::URI key(_model->world(), v->first.str());
		if (!skip_property(key)) {
			sratom_write(sratom, unmap, 0,
			             sord_node_to_serd_node(id.c_obj()),
			             sord_node_to_serd_node(key.c_obj()),
			             v->second.type(), v->second.size(), v->second.get_body());
		}
	}

	sord_inserter_free(inserter);
	serd_env_free(env);
	sratom_free(sratom);
}

} // namespace Serialisation
} // namespace Ingen
