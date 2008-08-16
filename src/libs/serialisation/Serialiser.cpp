/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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
#include <raul/Atom.hpp>
#include <raul/AtomRDF.hpp>
#include <raul/Path.hpp>
#include <raul/TableImpl.hpp>
#include <redlandmm/Model.hpp>
#include <redlandmm/Node.hpp>
#include <redlandmm/World.hpp>
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


Serialiser::Serialiser(Shared::World& world)
	: _store(world.store)
	, _world(*world.rdf_world)
{
}
	
void
Serialiser::to_file(SharedPtr<GraphObject> object, const string& filename)
{
	_root_object = object;
	start_to_filename(filename);
	serialise(object);
	finish();
}


	
string
Serialiser::to_string(SharedPtr<GraphObject>        object,
	                  const string&                 base_uri,
	                  const GraphObject::Variables& extra_rdf)
{
	_root_object = object;
	start_to_string(base_uri);
	serialise(object);
	
	Redland::Node base_rdf_node(_model->world(), Redland::Node::RESOURCE, base_uri);
	for (GraphObject::Variables::const_iterator v = extra_rdf.begin(); v != extra_rdf.end(); ++v) {
		if (v->first.find(":") != string::npos) {
			_model->add_statement(base_rdf_node, v->first,
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
 */
void
Serialiser::start_to_string(const string& base_uri)
{
	setlocale(LC_NUMERIC, "C");

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

	if (_mode == TO_FILE)
		_model->serialise_to_file(_base_uri);
	else
		ret = _model->serialise_to_string();

	_base_uri = "";
	_node_map.clear();
	
	return ret;
}


/** Convert a path to an RDF blank node ID for serializing.
 */
Redland::Node
Serialiser::path_to_node_id(const Path& path)
{
	assert(_model);
	
	if (path == _root_object->path())
		return Redland::Node(_model->world(), Redland::Node::RESOURCE, _base_uri);

	NodeMap::iterator i = _node_map.find(path);
	if (i != _node_map.end()) {
		assert(i->second);
		assert(i->second.get_node());
		return i->second;
	} else {
		Redland::Node id = _world.blank_id(Path::nameify(path.substr(1)));
		assert(id);
		_node_map[path] = id;
		return id;
	}
}


#if 0
/** Searches for the filename passed in the path, returning the full
 * path of the file, or the empty string if not found.
 *
 * This function tries to be as friendly a black box as possible - if the path
 * passed is an absolute path and the file is found there, it will return
 * that path, etc.
 *
 * additional_path is a list (colon delimeted as usual) of additional
 * directories to look in.  ie the directory the parent patch resides in would
 * be a good idea to pass as additional_path, in the case of a subpatch.
 */
string
Serialiser::find_file(const string& filename, const string& additional_path)
{
	string search_path = additional_path + ":" + _patch_search_path;
	
	// Try to open the raw filename first
	std::ifstream is(filename.c_str(), std::ios::in);
	if (is.good()) {
		is.close();
		return filename;
	}
	
	string directory;
	string full_patch_path = "";
	
	while (search_path != "") {
		directory = search_path.substr(0, search_path.find(':'));
		if (search_path.find(':') != string::npos)
			search_path = search_path.substr(search_path.find(':')+1);
		else
			search_path = "";

		full_patch_path = directory +"/"+ filename;
		
		std::ifstream is;
		is.open(full_patch_path.c_str(), std::ios::in);
	
		if (is.good()) {
			is.close();
			return full_patch_path;
		} else {
			cerr << "[Serialiser] Could not find patch file " << full_patch_path << endl;
		}
	}

	return "";
}
#endif

void
Serialiser::serialise(SharedPtr<GraphObject> object) throw (std::logic_error)
{
	if (!_model)
		throw std::logic_error("serialise called without serialization in progress");

	SharedPtr<Shared::Patch> patch = PtrCast<Shared::Patch>(object);
	if (patch) {
		serialise_patch(patch);
		return;
	}
	
	SharedPtr<Shared::Node> node = PtrCast<Shared::Node>(object);
	if (node) {
		serialise_node(node, path_to_node_id(node->path()));
		return;
	}
	
	SharedPtr<Shared::Port> port = PtrCast<Shared::Port>(object);
	if (port) {
		serialise_port(port.get(), path_to_node_id(port->path()));
		return;
	}

	cerr << "[Serialiser] WARNING: Unsupported object type, "
		<< object->path() << " not serialised." << endl;
}


Redland::Node
Serialiser::patch_path_to_rdf_id(const Path& path)
{
	if (path == _root_object->path()) {
		return Redland::Node(_model->world(), Redland::Node::RESOURCE, _base_uri);
	} else {
		assert(path.length() > _root_object->path().length());
		return Redland::Node(_model->world(), Redland::Node::RESOURCE,
				_base_uri + string("#") + path.substr(_root_object->path().length()));
	}
}


void
Serialiser::serialise_patch(SharedPtr<Shared::Patch> patch)
{
	assert(_model);

	const Redland::Node patch_id = patch_path_to_rdf_id(patch->path());
	
	_model->add_statement(
		patch_id,
		"rdf:type",
		Redland::Node(_model->world(), Redland::Node::RESOURCE, "http://drobilla.net/ns/ingen#Patch"));

	if (patch->path() != "/") {
		_model->add_statement(
			patch_id, "ingen:symbol",
			Redland::Node(_model->world(), Redland::Node::LITERAL, patch->path().name()));
	}

	_model->add_statement(
		patch_id,
		"ingen:polyphony",
		AtomRDF::atom_to_node(_model->world(), Atom((int)patch->internal_polyphony())));
	
	_model->add_statement(
		patch_id,
		"ingen:enabled",
		AtomRDF::atom_to_node(_model->world(), Atom((bool)patch->enabled())));
	
	serialise_variables(patch_id, patch->variables());

	//for (GraphObject::const_iterator n = patch->children_begin(); n != patch->children_end(); ++n) {
	for (GraphObject::const_iterator n = _store->children_begin(patch);
			n != _store->children_end(patch); ++n) {
		
		if (n->second->graph_parent() != patch.get())
			continue;

		SharedPtr<Shared::Patch> patch = PtrCast<Shared::Patch>(n->second);
		SharedPtr<Shared::Node>  node  = PtrCast<Shared::Node>(n->second);
		if (patch) {
			_model->add_statement(patch_id, "ingen:node", patch_path_to_rdf_id(patch->path()));
			serialise_patch(patch);
		} else if (node) {
			const Redland::Node node_id = path_to_node_id(n->second->path());
			_model->add_statement(patch_id, "ingen:node", node_id);
			serialise_node(node, node_id);
		}
	}
	
	for (uint32_t i=0; i < patch->num_ports(); ++i) {
		Port* p = patch->port(i);
		const Redland::Node port_id = path_to_node_id(p->path());
		_model->add_statement(patch_id, "ingen:port", port_id);
		serialise_port(p, port_id);
	}

	for (Shared::Patch::Connections::const_iterator c = patch->connections().begin();
			c != patch->connections().end(); ++c) {
		serialise_connection(*c);
	}
}


void
Serialiser::serialise_plugin(SharedPtr<Shared::Plugin> plugin)
{
	assert(_model);

	const Redland::Node plugin_id = Redland::Node(_model->world(), Redland::Node::RESOURCE, plugin->uri());

	_model->add_statement(
		plugin_id,
		"rdf:type",
		Redland::Node(_model->world(), Redland::Node::RESOURCE, plugin->type_uri()));
} 


void
Serialiser::serialise_node(SharedPtr<Shared::Node> node, const Redland::Node& node_id)
{
	const Redland::Node plugin_id
		= Redland::Node(_model->world(), Redland::Node::RESOURCE, node->plugin()->uri());

	_model->add_statement(
		node_id,
		"rdf:type",
		Redland::Node(_model->world(), Redland::Node::RESOURCE, "ingen:Node"));
	
	_model->add_statement(
		node_id,
		"ingen:symbol",
		Redland::Node(_model->world(), Redland::Node::LITERAL, node->path().name()));
	
	_model->add_statement(
		node_id,
		"ingen:plugin",
		plugin_id);
	
	_model->add_statement(
		node_id,
		"ingen:polyphonic",
		AtomRDF::atom_to_node(_model->world(), Atom(node->polyphonic())));

	//serialise_plugin(node->plugin());
	
	for (uint32_t i=0; i < node->num_ports(); ++i) {
		Port* p = node->port(i);
		assert(p);
		const Redland::Node port_id = path_to_node_id(p->path());
		serialise_port(p, port_id);
		_model->add_statement(node_id, "ingen:port", port_id);
	}

	serialise_variables(node_id, node->variables());
}


/** Writes a port subject with various information only if there are some
 * predicate/object pairs to go with it (eg if the port has variable, or a value, or..).
 * Audio output ports with no variable will not be written, for example.
 */
void
Serialiser::serialise_port(const Port* port, const Redland::Node& port_id)
{
	if (port->is_input())
		_model->add_statement(port_id, "rdf:type",
				Redland::Node(_model->world(), Redland::Node::RESOURCE, "ingen:InputPort"));
	else
		_model->add_statement(port_id, "rdf:type",
				Redland::Node(_model->world(), Redland::Node::RESOURCE, "ingen:OutputPort"));

	_model->add_statement(port_id, "ingen:symbol",
			Redland::Node(_model->world(), Redland::Node::LITERAL, port->path().name()));
	
	_model->add_statement(port_id, "rdf:type",
			Redland::Node(_model->world(), Redland::Node::RESOURCE, port->type().uri()));
	
	if (port->type() == DataType::CONTROL && port->is_input())
		_model->add_statement(port_id, "ingen:value",
				AtomRDF::atom_to_node(_model->world(), Atom(port->value())));

	serialise_variables(port_id, port->variables());
}


void
Serialiser::serialise_connection(SharedPtr<Connection> connection) throw (std::logic_error)
{
	if (!_model)
		throw std::logic_error("serialise_connection called without serialization in progress");

	const Redland::Node src_node = path_to_node_id(connection->src_port_path());
	const Redland::Node dst_node = path_to_node_id(connection->dst_port_path());

	/* This would allow associating data with the connection... */
	/*const Redland::Node connection_node = _world.blank_id();
	_model->add_statement(connection_node, "ingen:hasSource", src_node);
	_model->add_statement(dst_node, "ingen:hasConnection", connection_node);
	*/

	/* ... but this is cleaner */
	_model->add_statement(dst_node, "ingen:connectedTo", src_node);
}
	

void
Serialiser::serialise_variables(Redland::Node subject, const GraphObject::Variables& variables)
{
	for (GraphObject::Variables::const_iterator v = variables.begin(); v != variables.end(); ++v) {
		if (v->first.find(":") != string::npos && v->first != "ingen:document") {
			if (v->second.is_valid()) {
				const Redland::Node var_id = _world.blank_id();
				const Redland::Node key(_model->world(), Redland::Node::RESOURCE, v->first);
				const Redland::Node value = AtomRDF::atom_to_node(_model->world(), v->second);
				if (value) {
					_model->add_statement(subject, "ingen:variable", var_id);
					_model->add_statement(var_id, "ingen:key", key);
					_model->add_statement(var_id, "ingen:value", value);
				} else {
					cerr << "Warning: can not serialise value: key '" << v->first << "', type "
						<< (int)v->second.type() << endl;
				}
			} else {
				cerr << "Warning: variable with no value: key '" << v->first << "'" << endl;
			}
		} else {
			cerr << "Warning: not serialising variable with invalid key '" << v->first << "'" << endl;
		}
	}
}


} // namespace Serialisation
} // namespace Ingen
