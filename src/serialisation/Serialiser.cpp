/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib> // atof
#include <cstring>
#include <fstream>
#include <iostream>
#include <locale.h>
#include <stdexcept>
#include <string>
#include <utility> // pair, make_pair
#include <vector>
#include <glib.h>
#include <glib/gstdio.h>
#include <glibmm/convert.h>
#include "raul/Atom.hpp"
#include "raul/AtomRDF.hpp"
#include "raul/Path.hpp"
#include "raul/TableImpl.hpp"
#include "redlandmm/Model.hpp"
#include "redlandmm/Node.hpp"
#include "redlandmm/World.hpp"
#include "module/World.hpp"
#include "interface/EngineInterface.hpp"
#include "interface/Plugin.hpp"
#include "interface/Patch.hpp"
#include "interface/Node.hpp"
#include "interface/Port.hpp"
#include "interface/Connection.hpp"
#include "Serialiser.hpp"

using namespace std;
using namespace Raul;
using namespace Redland;
using namespace Ingen;
using namespace Ingen::Shared;

namespace Ingen {
namespace Serialisation {


#define META_PREFIX "#"


Serialiser::Serialiser(Shared::World& world, SharedPtr<Shared::Store> store)
	: _root_path("/")
	, _store(store)
	, _world(*world.rdf_world)
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
	string symbol = uri;
	symbol = symbol.substr(symbol.find_last_of("/") + 1);
	symbol = symbol.substr(0, symbol.find("."));
	symbol = Path::nameify(symbol);
	return symbol;
}


void
Serialiser::write_manifest(const std::string& bundle_uri,
                           const Records&     records)
{
	const string filename = Glib::filename_from_uri(bundle_uri) + "manifest.ttl";
	start_to_filename(filename);
    _model->set_base_uri(bundle_uri);
	for (Records::const_iterator i = records.begin(); i != records.end(); ++i) {
		SharedPtr<Patch> patch = PtrCast<Patch>(i->object);
		if (patch) {
			const Redland::Resource subject(_model->world(), uri_to_symbol(i->uri));
			_model->add_statement(subject, "rdf:type",
				Redland::Resource(_model->world(), "ingen:Patch"));
			_model->add_statement(subject, "rdf:type",
				Redland::Resource(_model->world(), "lv2:Plugin"));
			_model->add_statement(subject, "rdfs:seeAlso",
				Redland::Resource(_model->world(), i->uri));
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

	const string root_file = bundle_uri + symbol + ".ingen.ttl";
	start_to_filename(root_file);
	serialise(object);
	finish();
	records.push_back(Record(object, bundle_uri + symbol + ".ingen.ttl"));
	write_manifest(bundle_uri, records);
}


string
Serialiser::to_string(SharedPtr<GraphObject>         object,
	                  const string&                  base_uri,
	                  const GraphObject::Properties& extra_rdf)
{
	start_to_string(object->path(), base_uri);
	serialise(object);

	Redland::Resource base_rdf_node(_model->world(), base_uri);
	for (GraphObject::Properties::const_iterator v = extra_rdf.begin(); v != extra_rdf.end(); ++v) {
		if (v->first.find(":") != string::npos) {
			_model->add_statement(base_rdf_node, v->first.str(),
					AtomRDF::atom_to_node(_model->world(), v->second));
		} else {
			cerr << "Warning: not serialising extra RDF with key '" << v->first << "'" << endl;
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
	_model = new Redland::Model(_world);
    _model->set_base_uri(_base_uri);
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
	_model = new Redland::Model(_world);
    _model->set_base_uri(base_uri);
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
		_model->serialise_to_file(_base_uri);
	} else {
		char* c_str = _model->serialise_to_string();
		if (c_str != NULL) {
			ret = c_str;
			free(c_str);
		}
	}

	_base_uri = "";
	return ret;
}


Redland::Node
Serialiser::instance_rdf_node(const Path& path)
{
	assert(_model);
	assert(path.is_child_of(_root_path));

	if (path == _root_path)
		return Redland::Resource(_model->world(), _base_uri);
	else
		return Redland::Resource(_model->world(),
				path.substr(_root_path.base().length()));
}


Redland::Node
Serialiser::class_rdf_node(const Path& path)
{
	assert(_model);
	assert(path.is_child_of(_root_path));

	if (path == _root_path)
		return Redland::Resource(_model->world(), _base_uri);
	else
		return Redland::Resource(_model->world(),
				string(META_PREFIX) + path.relative_to_base(_root_path).chop_start("/"));
}


void
Serialiser::serialise(SharedPtr<GraphObject> object) throw (std::logic_error)
{
	if (!_model)
		throw std::logic_error("serialise called without serialization in progress");

	SharedPtr<Shared::Patch> patch = PtrCast<Shared::Patch>(object);
	if (patch) {
		if (patch->path() == _root_path) {
			const Redland::Resource patch_id(_model->world(), _base_uri);
			serialise_patch(patch, patch_id);
		} else {
			const Redland::Resource patch_id(_model->world(),
					string(META_PREFIX) + patch->path().chop_start("/"));
			serialise_patch(patch, patch_id);
			serialise_node(patch, patch_id, instance_rdf_node(patch->path()));
		}
		return;
	}

	SharedPtr<Shared::Node> node = PtrCast<Shared::Node>(object);
	if (node) {
		const Redland::Resource plugin_id(_model->world(), node->plugin()->uri().str());
		serialise_node(node, plugin_id, instance_rdf_node(node->path()));
		return;
	}

	SharedPtr<Shared::Port> port = PtrCast<Shared::Port>(object);
	if (port) {
		serialise_port(port.get(), instance_rdf_node(port->path()));
		return;
	}

	cerr << "[Serialiser] WARNING: Unsupported object type, "
		<< object->path() << " not serialised." << endl;
}


void
Serialiser::serialise_patch(SharedPtr<Shared::Patch> patch, const Redland::Node& patch_id)
{
	assert(_model);

	_model->add_statement(patch_id, "rdf:type",
			Redland::Resource(_model->world(), "ingen:Patch"));

	_model->add_statement(patch_id, "rdf:type",
			Redland::Resource(_model->world(), "lv2:Plugin"));

	GraphObject::Properties::const_iterator s = patch->properties().find("lv2:symbol");
	// If symbol is stored as a property, write that
	if (s != patch->properties().end() && s->second.is_valid()) {
		_model->add_statement(patch_id, "lv2:symbol",
			Redland::Literal(_model->world(), s->second.get_string()));
	// Otherwise take the one from our path (if possible)
	} else if (!patch->path().is_root()) {
		_model->add_statement(patch_id, "lv2:symbol",
				Redland::Literal(_model->world(), patch->path().name()));
	} else {
		cerr << "WARNING: Patch has no lv2:symbol" << endl;
	}

	serialise_properties(patch_id, patch->meta().properties());

	for (Store::const_iterator n = _store->children_begin(patch);
			n != _store->children_end(patch); ++n) {

		if (n->second->graph_parent() != patch.get())
			continue;

		SharedPtr<Shared::Patch> patch = PtrCast<Shared::Patch>(n->second);
		SharedPtr<Shared::Node>  node  = PtrCast<Shared::Node>(n->second);
		if (patch) {
			const Redland::Resource class_id(_model->world(),
					string(META_PREFIX) + patch->path().chop_start("/"));
			const Redland::Node     node_id(instance_rdf_node(n->second->path()));
			_model->add_statement(patch_id, "ingen:node", node_id);
			serialise_patch(patch, class_id);
			serialise_node(patch, class_id, node_id);
		} else if (node) {
			const Redland::Resource class_id(_model->world(), node->plugin()->uri().str());
			const Redland::Node     node_id(instance_rdf_node(n->second->path()));
			_model->add_statement(patch_id, "ingen:node", node_id);
			serialise_node(node, class_id, node_id);
		}
	}

	bool root = (patch->path() == _root_path);

	for (uint32_t i=0; i < patch->num_ports(); ++i) {
		Port* p = patch->port(i);
		const Redland::Node port_id = root
			? instance_rdf_node(p->path())
			: class_rdf_node(p->path());

		// Ensure lv2:name always exists so Patch is a valid LV2 plugin
		if (p->properties().find("lv2:name") == p->properties().end())
			p->set_property("lv2:name", Atom(Atom::STRING, p->symbol()));

		_model->add_statement(patch_id, "lv2:port", port_id);
		serialise_port_meta(p, port_id);
		if (root)
			serialise_properties(port_id, p->properties());
	}

	for (Shared::Patch::Connections::const_iterator c = patch->connections().begin();
			c != patch->connections().end(); ++c) {
		serialise_connection(patch, *c);
	}
}


void
Serialiser::serialise_plugin(const Shared::Plugin& plugin)
{
	assert(_model);

	const Redland::Node plugin_id = Redland::Resource(_model->world(), plugin.uri().str());

	_model->add_statement(plugin_id, "rdf:type",
		Redland::Resource(_model->world(), plugin.type_uri()));
}


void
Serialiser::serialise_node(SharedPtr<Shared::Node> node,
		const Redland::Node& class_id, const Redland::Node& node_id)
{
	_model->add_statement(node_id, "rdf:type",
			Redland::Resource(_model->world(), "ingen:Node"));
	_model->add_statement(node_id, "rdf:instanceOf",
			class_id);
	_model->add_statement(node_id, "lv2:symbol",
			Redland::Literal(_model->world(), node->path().name()));

	serialise_properties(node_id, node->properties());

	for (uint32_t i=0; i < node->num_ports(); ++i) {
		Port* p = node->port(i);
		const Redland::Node port_id = instance_rdf_node(p->path());
		serialise_port(p, port_id);
		_model->add_statement(node_id, "lv2:port", port_id);
	}
}


/** Serialise a port on a Node */
void
Serialiser::serialise_port(const Port* port, const Redland::Node& port_id)
{
	if (port->is_input())
		_model->add_statement(port_id, "rdf:type",
				Redland::Resource(_model->world(), "lv2:InputPort"));
	else
		_model->add_statement(port_id, "rdf:type",
				Redland::Resource(_model->world(), "lv2:OutputPort"));

	_model->add_statement(port_id, "rdf:type",
			Redland::Resource(_model->world(), port->type().uri()));

	if (dynamic_cast<Patch*>(port->graph_parent()))
		_model->add_statement(port_id, "rdf:instanceOf",
				class_rdf_node(port->path()));

	if (port->is_input() && port->type() == PortType::CONTROL)
		_model->add_statement(port_id, "ingen:value",
				AtomRDF::atom_to_node(_model->world(), Atom(port->value())));

	serialise_properties(port_id, port->properties());
}


/** Serialise a port on a Patch */
void
Serialiser::serialise_port_meta(const Port* port, const Redland::Node& port_id)
{
	if (port->is_input())
		_model->add_statement(port_id, "rdf:type",
				Redland::Resource(_model->world(), "lv2:InputPort"));
	else
		_model->add_statement(port_id, "rdf:type",
				Redland::Resource(_model->world(), "lv2:OutputPort"));

	_model->add_statement(port_id, "rdf:type",
			Redland::Resource(_model->world(), port->type().uri()));

	_model->add_statement(port_id, "lv2:index",
			AtomRDF::atom_to_node(_model->world(), Atom((int)port->index())));

	if (!port->get_property("lv2:default").is_valid()) {
		if (port->is_input()) {
			if (port->value().is_valid()) {
				_model->add_statement(port_id, "lv2:default",
						AtomRDF::atom_to_node(_model->world(), Atom(port->value())));
			} else if (port->type() == PortType::CONTROL) {
				cerr << "WARNING: Port " << port->path() << " has no lv2:default" << endl;
			}
		}
	}

	serialise_properties(port_id, port->meta().properties());
}


void
Serialiser::serialise_connection(SharedPtr<GraphObject> parent,
                                 SharedPtr<Connection>  connection) throw (std::logic_error)
{
	if (!_model)
		throw std::logic_error("serialise_connection called without serialization in progress");

	bool top = (parent->path() == _root_path);
	const Redland::Node src_node = top
		? instance_rdf_node(connection->src_port_path())
		: class_rdf_node(connection->src_port_path());
	const Redland::Node dst_node = top
		? instance_rdf_node(connection->dst_port_path())
		: class_rdf_node(connection->dst_port_path());

	const Redland::Node connection_node = _world.blank_id();
	_model->add_statement(connection_node, "ingen:source", src_node);
	_model->add_statement(connection_node, "ingen:destination", dst_node);
	if (parent) {
		const Redland::Node parent_node = class_rdf_node(parent->path());
		_model->add_statement(parent_node, "ingen:connection", connection_node);
	} else {
		_model->add_statement(connection_node, "rdf:type",
				Redland::Resource(_model->world(), "ingen:Connection"));
	}
}


void
Serialiser::serialise_properties(
		Redland::Node                  subject,
		const GraphObject::Properties& properties)
{
	for (GraphObject::Properties::const_iterator v = properties.begin(); v != properties.end(); ++v) {
		if (v->second.is_valid()) {
			const Redland::Resource key(_model->world(), v->first.str());
			const Redland::Node value = AtomRDF::atom_to_node(_model->world(), v->second);
			if (value.is_valid()) {
				_model->add_statement(subject, key, value);
			} else {
				cerr << "WARNING: can not serialise variable '" << v->first << "' :: "
					<< (int)v->second.type() << endl;
			}
		} else {
			cerr << "WARNING: property '" << v->first << "' has no value" << endl;
		}
	}
}


} // namespace Serialisation
} // namespace Ingen
