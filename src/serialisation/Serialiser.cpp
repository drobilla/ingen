/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <glib.h>
#include <glib/gstdio.h>
#include <glibmm/convert.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>

#include "raul/Atom.hpp"
#include "raul/AtomRDF.hpp"
#include "raul/Path.hpp"
#include "raul/TableImpl.hpp"
#include "raul/log.hpp"

#include "sord/sordmm.hpp"

#include "interface/Connection.hpp"
#include "interface/EngineInterface.hpp"
#include "interface/Node.hpp"
#include "interface/Patch.hpp"
#include "interface/Plugin.hpp"
#include "interface/Port.hpp"
#include "module/World.hpp"
#include "shared/LV2URIMap.hpp"
#include "shared/ResourceImpl.hpp"

#include "Serialiser.hpp"
#include "names.hpp"

#define LOG(s) s << "[Serialiser] "

#define NS_LV2 "http://lv2plug.in/ns/lv2core#"

using namespace std;
using namespace Raul;
using namespace Sord;
using namespace Ingen;
using namespace Ingen::Shared;

namespace Ingen {
namespace Serialisation {

#define META_PREFIX "#"

Serialiser::Serialiser(Shared::World& world, SharedPtr<Shared::Store> store)
	: _root_path("/")
	, _store(store)
	, _world(world)
{
}

void
Serialiser::to_file(const Record& record)
{
	SharedPtr<GraphObject> object = record.object;
	const string& filename = record.uri;

	_root_path = object->path();
	start_to_filename(filename);
	serialise(object);
	finish();
}

static
std::string
uri_to_symbol(const std::string& uri)
{
	return Path::nameify(Glib::path_get_basename(Glib::filename_from_uri(uri)));
}

void
Serialiser::write_manifest(const std::string& bundle_uri,
                           const Records&     records)
{
	const string bundle_path(Glib::filename_from_uri(bundle_uri));
	const string filename(Glib::build_filename(bundle_path, "manifest.ttl"));
	start_to_filename(filename);
	Sord::World& world = _model->world();
	for (Records::const_iterator i = records.begin(); i != records.end(); ++i) {
		SharedPtr<Patch> patch = PtrCast<Patch>(i->object);
		if (patch) {
			const std::string filename = uri_to_symbol(i->uri) + INGEN_PATCH_FILE_EXT;
			const Sord::URI   subject(world, filename);
			_model->add_statement(subject,
			                      Sord::Curie(world, "rdf:type"),
			                      Sord::Curie(world, "ingen:Patch"));
			_model->add_statement(subject,
			                      Sord::Curie(world, "rdf:type"),
			                      Sord::Curie(world, "lv2:Plugin"));
			_model->add_statement(subject,
			                      Sord::Curie(world, "rdfs:seeAlso"),
			                      Sord::URI(world, filename));
			_model->add_statement(
				subject,
				Sord::Curie(world, "lv2:binary"),
				Sord::URI(world, Glib::Module::build_path("", "ingen_lv2")));
			symlink(Glib::Module::build_path(INGEN_MODULE_DIR, "ingen_lv2").c_str(),
			        Glib::Module::build_path(bundle_path, "ingen_lv2").c_str());
		}
	}
	finish();
}

void
Serialiser::write_bundle(const Record& record)
{
	SharedPtr<GraphObject> object = record.object;
	string bundle_uri = record.uri;
	if (bundle_uri[bundle_uri.length()-1] != '/')
		bundle_uri.append("/");

	g_mkdir_with_parents(Glib::filename_from_uri(bundle_uri).c_str(), 0744);
	Records records;

	string symbol = uri_to_symbol(record.uri);

	const string root_file = bundle_uri + symbol + INGEN_PATCH_FILE_EXT;
	start_to_filename(root_file);
	serialise(object);
	finish();
	records.push_back(Record(object, bundle_uri + symbol + INGEN_PATCH_FILE_EXT));
	write_manifest(bundle_uri, records);
}

string
Serialiser::to_string(SharedPtr<GraphObject>         object,
                      const string&                  base_uri,
                      const GraphObject::Properties& extra_rdf)
{
	start_to_string(object->path(), base_uri);
	serialise(object);

	Sord::URI base_rdf_node(_model->world(), base_uri);
	for (GraphObject::Properties::const_iterator v = extra_rdf.begin();
	     v != extra_rdf.end(); ++v) {
		if (v->first.find(":") != string::npos) {
			_model->add_statement(base_rdf_node,
			                      AtomRDF::atom_to_node(*_model, v->first),
			                      AtomRDF::atom_to_node(*_model, v->second));
		} else {
			LOG(warn) << "Not serialising extra RDF with key '" << v->first << "'" << endl;
		}
	}

	return finish();
}

/** Begin a serialization to a file.
 *
 * This must be called before any serializing methods.
 */
void
Serialiser::start_to_filename(const string& filename)
{
	setlocale(LC_NUMERIC, "C");

	assert(filename.find(":") == string::npos || filename.substr(0, 5) == "file:");
	if (filename.find(":") == string::npos)
		_base_uri = "file://" + filename;
	else
		_base_uri = filename;
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
	setlocale(LC_NUMERIC, "C");

	_root_path = root;
	_base_uri = base_uri;
	_model = new Sord::Model(*_world.rdf_world(), base_uri);
	_mode = TO_STRING;
}

/** Finish a serialization.
 *
 * If this was a serialization to a string, the serialization output
 * will be returned, otherwise the empty string is returned.
 */
string
Serialiser::finish()
{
	string ret = "";
	if (_mode == TO_FILE) {
		_model->write_to_file(_base_uri, "turtle");
	} else {
		char* c_str = _model->write_to_string("turtle");
		if (c_str != NULL) {
			ret = c_str;
			free(c_str);
		}
	}

	delete _model;
	_base_uri = "";

	return ret;
}

Sord::Node
Serialiser::path_rdf_node(const Path& path)
{
	assert(_model);
	assert(path.is_child_of(_root_path));
	return Sord::URI(_model->world(), path.chop_scheme().substr(1));
}

void
Serialiser::serialise(SharedPtr<GraphObject> object) throw (std::logic_error)
{
	if (!_model)
		throw std::logic_error("serialise called without serialization in progress");

	SharedPtr<Shared::Patch> patch = PtrCast<Shared::Patch>(object);
	if (patch) {
		if (patch->path() == _root_path) {
			const Sord::URI patch_id(_model->world(), "");
			serialise_patch(patch, patch_id);
		} else {
			const Sord::URI patch_id(_model->world(),
			                         string(META_PREFIX) + patch->path().chop_start("/"));
			serialise_patch(patch, patch_id);
			serialise_node(patch, patch_id, path_rdf_node(patch->path()));
		}
		return;
	}

	SharedPtr<Shared::Node> node = PtrCast<Shared::Node>(object);
	if (node) {
		const Sord::URI plugin_id(_model->world(), node->plugin()->uri().str());
		serialise_node(node, plugin_id, path_rdf_node(node->path()));
		return;
	}

	SharedPtr<Shared::Port> port = PtrCast<Shared::Port>(object);
	if (port) {
		serialise_port(port.get(), path_rdf_node(port->path()));
		return;
	}

	LOG(warn) << "Unsupported object type, "
	          << object->path() << " not serialised." << endl;
}

void
Serialiser::serialise_patch(SharedPtr<Shared::Patch> patch, const Sord::Node& patch_id)
{
	assert(_model);
	Sord::World& world = _model->world();

	_model->add_statement(patch_id,
	                      Sord::Curie(world, "rdf:type"),
	                      Sord::Curie(world, "ingen:Patch"));

	_model->add_statement(patch_id,
	                      Sord::Curie(world, "rdf:type"),
	                      Sord::Curie(world, "lv2:Plugin"));

	const LV2URIMap& uris = *_world.uris().get();

	// Always write a symbol (required by Ingen)
	string symbol;
	GraphObject::Properties::const_iterator s = patch->properties().find(uris.lv2_symbol);
	if (s == patch->properties().end()
	    || !s->second.type() == Atom::STRING
	    || !Symbol::is_valid(s->second.get_string())) {
		symbol = Glib::path_get_basename(Glib::filename_from_uri(_model->base_uri().to_c_string()));
		symbol = Symbol::symbolify(symbol.substr(0, symbol.find('.')));
		_model->add_statement(
			patch_id,
			AtomRDF::atom_to_node(*_model, uris.lv2_symbol.c_str()),
			Sord::Literal(world, symbol));
	} else {
		symbol = s->second.get_string();
	}

	// If the patch has no doap:name (required by LV2), use the symbol
	if (patch->meta().properties().find(uris.doap_name) == patch->meta().properties().end())
		_model->add_statement(patch_id,
		                      AtomRDF::atom_to_node(*_model, uris.doap_name),
		                      Sord::Literal(world, symbol));

	serialise_properties(patch_id, NULL, patch->meta().properties());

	for (Store::const_iterator n = _store->children_begin(patch);
	     n != _store->children_end(patch); ++n) {

		if (n->first.parent() != patch->path())
			continue;

		SharedPtr<Shared::Patch> subpatch = PtrCast<Shared::Patch>(n->second);
		SharedPtr<Shared::Node>  node  = PtrCast<Shared::Node>(n->second);
		if (subpatch) {
			const Sord::URI class_id(world,
			                         string(META_PREFIX) + subpatch->path().chop_start("/"));
			const Sord::Node     node_id(path_rdf_node(n->second->path()));
			_model->add_statement(patch_id,
			                      Sord::Curie(world, "ingen:node"),
			                      node_id);
			serialise_patch(subpatch, class_id);
			serialise_node(subpatch, class_id, node_id);
		} else if (node) {
			const Sord::URI  class_id(world, node->plugin()->uri().str());
			const Sord::Node node_id(path_rdf_node(n->second->path()));
			_model->add_statement(patch_id,
			                      Sord::Curie(world, "ingen:node"),
			                      node_id);
			serialise_node(node, class_id, node_id);
		}
	}

	bool root = (patch->path() == _root_path);

	for (uint32_t i=0; i < patch->num_ports(); ++i) {
		Port* p = patch->port(i);
		const Sord::Node port_id = path_rdf_node(p->path());

		// Ensure lv2:name always exists so Patch is a valid LV2 plugin
		if (p->properties().find(NS_LV2 "name") == p->properties().end())
			p->set_property(NS_LV2 "name", Atom(p->symbol().c_str()));

		_model->add_statement(patch_id,
		                      Sord::URI(world, NS_LV2 "port"),
		                      port_id);
		serialise_port_meta(p, port_id);
		if (root)
			serialise_properties(port_id, &p->meta(), p->properties());
	}

	for (Shared::Patch::Connections::const_iterator c = patch->connections().begin();
	     c != patch->connections().end(); ++c) {
		serialise_connection(patch, c->second);
	}
}

void
Serialiser::serialise_plugin(const Shared::Plugin& plugin)
{
	assert(_model);

	const Sord::Node plugin_id = Sord::URI(_model->world(), plugin.uri().str());

	_model->add_statement(plugin_id,
	                      Sord::Curie(_model->world(), "rdf:type"),
	                      Sord::URI(_model->world(), plugin.type_uri().str()));
}

void
Serialiser::serialise_node(SharedPtr<Shared::Node> node,
                           const Sord::Node&       class_id,
                           const Sord::Node&       node_id)
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

	serialise_properties(node_id, &node->meta(), node->properties());

	for (uint32_t i = 0; i < node->num_ports(); ++i) {
		Port* const      p       = node->port(i);
		const Sord::Node port_id = path_rdf_node(p->path());
		serialise_port(p, port_id);
		_model->add_statement(node_id,
		                      Sord::Curie(_model->world(), "lv2:port"),
		                      port_id);
	}
}

/** Serialise a port on a Node */
void
Serialiser::serialise_port(const Port* port, const Sord::Node& port_id)
{
	Sord::World& world = _model->world();

	if (port->is_input())
		_model->add_statement(port_id,
		                      Sord::Curie(world, "rdf:type"),
		                      Sord::Curie(world, "lv2:InputPort"));
	else
		_model->add_statement(port_id,
		                      Sord::Curie(world, "rdf:type"),
		                      Sord::Curie(world, "lv2:OutputPort"));

	for (Port::PortTypes::const_iterator i = port->types().begin();
	     i != port->types().end(); ++i)
		_model->add_statement(port_id,
		                      Sord::Curie(world, "rdf:type"),
		                      Sord::URI(world, i->uri().str()));

	/*
	if (dynamic_cast<Patch*>(port->graph_parent()))
		_model->add_statement(port_id,
		                      Sord::Curie(world, "rdf:instanceOf"),
		                      class_rdf_node(port->path()));
	*/

	_model->add_statement(port_id,
	                      Sord::Curie(world, "lv2:symbol"),
	                      Sord::Literal(world, port->path().symbol()));

	serialise_properties(port_id, &port->meta(), port->properties());
}

/** Serialise a port on a Patch */
void
Serialiser::serialise_port_meta(const Port* port, const Sord::Node& port_id)
{
	Sord::World& world = _model->world();

	if (port->is_input())
		_model->add_statement(port_id,
		                      Sord::Curie(world, "rdf:type"),
		                      Sord::Curie(world, "lv2:InputPort"));
	else
		_model->add_statement(port_id,
		                      Sord::Curie(world, "rdf:type"),
		                      Sord::Curie(world, "lv2:OutputPort"));

	for (Port::PortTypes::const_iterator i = port->types().begin();
	     i != port->types().end(); ++i)
		_model->add_statement(port_id,
		                      Sord::Curie(world, "rdf:type"),
		                      Sord::URI(world, i->uri().str()));

	_model->add_statement(
		port_id,
		Sord::Curie(world, "lv2:index"),
		AtomRDF::atom_to_node(*_model, Atom((int)port->index())));

	_model->add_statement(
		port_id,
		Sord::Curie(world, "lv2:symbol"),
		Sord::Literal(world, port->path().symbol()));

	if (!port->get_property(NS_LV2 "default").is_valid()) {
		if (port->is_input()) {
			if (port->value().is_valid()) {
				_model->add_statement(
					port_id,
					Sord::Curie(world, "lv2:default"),
					AtomRDF::atom_to_node(*_model, port->value()));
			} else if (port->is_a(PortType::CONTROL)) {
				LOG(warn) << "Port " << port->path() << " has no lv2:default" << endl;
			}
		}
	}

	serialise_properties(port_id, NULL, port->meta().properties());
}

void
Serialiser::serialise_connection(SharedPtr<GraphObject> parent,
                                 SharedPtr<Connection>  connection) throw (std::logic_error)
{
	Sord::World& world = _model->world();

	if (!_model)
		throw std::logic_error(
			"serialise_connection called without serialization in progress");

	const Sord::Node src           = path_rdf_node(connection->src_port_path());
	const Sord::Node dst           = path_rdf_node(connection->dst_port_path());
	const Sord::Node connection_id = Sord::Node::blank_id(*_world.rdf_world());
	_model->add_statement(connection_id,
	                      Sord::Curie(world, "ingen:source"),
	                      src);
	_model->add_statement(connection_id,
	                      Sord::Curie(world, "ingen:destination"),
	                      dst);
	/*
	if (parent) {
		const Sord::Node parent_id = class_rdf_node(parent->path());
		_model->add_statement(parent_id,
		                      Sord::Curie(world, "ingen:connection"),
		                      connection_id);
	} else {
	*/
		_model->add_statement(connection_id,
		                      Sord::Curie(world, "rdf:type"),
		                      Sord::Curie(world, "ingen:Connection"));
		//}
}

void
Serialiser::serialise_properties(Sord::Node                     subject,
                                 const Shared::Resource*        meta,
                                 const GraphObject::Properties& properties)
{
	for (GraphObject::Properties::const_iterator v = properties.begin();
	     v != properties.end(); ++v) {
		if (v->second.is_valid()) {
			if (!meta || !meta->has_property(v->first.str(), v->second)) {
				const Sord::URI  key(_model->world(), v->first.str());
				const Sord::Node value(AtomRDF::atom_to_node(*_model, v->second));
				if (value.is_valid()) {
					_model->add_statement(subject, key, value);
				} else {
					LOG(warn) << "Can not serialise variable '" << v->first << "' :: "
					          << (int)v->second.type() << endl;
				}
			}
		} else {
			LOG(warn) << "Property '" << v->first << "' has no value" << endl;
		}
	}
}

} // namespace Serialisation
} // namespace Ingen
