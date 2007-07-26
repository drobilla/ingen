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
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <utility> // pair, make_pair
#include <stdexcept>
#include <cassert>
#include <cmath>
#include <cstdlib> // atof
#include <boost/optional/optional.hpp>
#include <cstring>
#include <locale.h>
#include <raul/RDFWorld.hpp>
#include <raul/RDFModel.hpp>
#include <raul/RDFNode.hpp>
#include <raul/Path.hpp>
#include <raul/Atom.hpp>
#include <raul/AtomRedland.hpp>
#include <raul/TableImpl.hpp>
#include "interface/EngineInterface.hpp"
#include "Serializer.hpp"
#include "PatchModel.hpp"
#include "NodeModel.hpp"
#include "ConnectionModel.hpp"
#include "PortModel.hpp"
#include "PresetModel.hpp"
#include "PluginModel.hpp"

using namespace std;
using namespace Raul;
using namespace Raul::RDF;
using boost::optional;

namespace Ingen {
namespace Client {


Serializer::Serializer(Raul::RDF::World& world)
	: _world(world)
{
}
	
void
Serializer::to_file(SharedPtr<ObjectModel> object, const string& filename)
{
	_root_object = object;
	start_to_filename(filename);
	serialize(object);
	finish();
}


string
Serializer::to_string(SharedPtr<ObjectModel> object)
{
	_root_object = object;
	start_to_string();
	serialize(object);
	return finish();
}


/** Begin a serialization to a file.
 *
 * This must be called before any serializing methods.
 */
void
Serializer::start_to_filename(const string& filename)
{
	setlocale(LC_NUMERIC, "C");

	_base_uri = "file://" + filename;
	_model = new RDF::Model(_world);
	_mode = TO_FILE;
}


/** Begin a serialization to a string.
 *
 * This must be called before any serializing methods.
 *
 * The results of the serialization will be returned by the finish() method after
 * the desired objects have been serialized.
 */
void
Serializer::start_to_string()
{
	setlocale(LC_NUMERIC, "C");

	_base_uri = "";
	_model = new RDF::Model(_world);
	_mode = TO_STRING;
}


/** Finish a serialization.
 *
 * If this was a serialization to a string, the serialization output
 * will be returned, otherwise the empty string is returned.
 */
string
Serializer::finish()
{
	string ret = "";

	if (_mode == TO_FILE)
		_model->serialize_to_file(_base_uri);
	else
		ret = _model->serialize_to_string();

	_base_uri = "";
	_node_map.clear();
	
	return ret;
}


/** Convert a path to an RDF blank node ID for serializing.
 */
Node
Serializer::path_to_node_id(const Path& path)
{
	assert(_model);
	/*string ret = path.substr(1);

	for (size_t i=0; i < ret.length(); ++i) {
		if (ret[i] == '/')
			ret[i] = '_';
	}

	return Node(Node::BLANK, ret);
	*/

	NodeMap::iterator i = _node_map.find(path);
	if (i != _node_map.end()) {
		return i->second;
	} else {
		Node id = _world.blank_id();
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
Serializer::find_file(const string& filename, const string& additional_path)
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
			cerr << "[Serializer] Could not find patch file " << full_patch_path << endl;
		}
	}

	return "";
}
#endif

void
Serializer::serialize(SharedPtr<ObjectModel> object) throw (std::logic_error)
{
	if (!_model)
		throw std::logic_error("serialize called without serialization in progress");

	SharedPtr<PatchModel> patch = PtrCast<PatchModel>(object);
	if (patch) {
		serialize_patch(patch);
		return;
	}
	
	SharedPtr<NodeModel> node = PtrCast<NodeModel>(object);
	if (node) {
		serialize_node(node, path_to_node_id(node->path()));
		return;
	}
	
	SharedPtr<PortModel> port = PtrCast<PortModel>(object);
	if (port) {
		serialize_port(port, path_to_node_id(port->path()));
		return;
	}

	cerr << "[Serializer] WARNING: Unsupported object type, "
		<< object->path() << " not serialized." << endl;
}


Node
Serializer::patch_path_to_rdf_id(const Path& path)
{
	if (path == _root_object->path()) {
		return Node(_model->world(), Node::RESOURCE, _base_uri);
	} else {
		assert(path.length() > _root_object->path().length());
		return Node(_model->world(), Node::RESOURCE,
				_base_uri + string("#") + path.substr(_root_object->path().length() + 1));
	}
}


void
Serializer::serialize_patch(SharedPtr<PatchModel> patch)
{
	assert(_model);

	const Node patch_id = patch_path_to_rdf_id(patch->path());
	
	_model->add_statement(
		patch_id,
		"rdf:type",
		Node(_model->world(), Node::RESOURCE, "http://drobilla.net/ns/ingen#Patch"));

	if (patch->path().name().length() > 0) {
		_model->add_statement(
			patch_id, "ingen:name",
			Atom(patch->path().name().c_str()));
	}

	_model->add_statement(
		patch_id,
		"ingen:polyphony",
		Atom((int)patch->poly()));
	
	for (MetadataMap::const_iterator m = patch->metadata().begin(); m != patch->metadata().end(); ++m) {
		if (m->first.find(":") != string::npos) {
			_model->add_statement(
				patch_id,
				m->first.c_str(),
				m->second);
		}
	}

	for (ObjectModel::Children::const_iterator n = patch->children().begin(); n != patch->children().end(); ++n) {
		SharedPtr<PatchModel> patch = PtrCast<PatchModel>(n->second);
		SharedPtr<NodeModel> node   = PtrCast<NodeModel>(n->second);
		if (patch) {
			_model->add_statement(patch_id, "ingen:node", patch_path_to_rdf_id(patch->path()));
			serialize_patch(patch);
		} else if (node) {
			const Node node_id = path_to_node_id(n->second->path());
			_model->add_statement(patch_id, "ingen:node", node_id);
			serialize_node(node, node_id);
		}
	}
	
	for (PortModelList::const_iterator p = patch->ports().begin(); p != patch->ports().end(); ++p) {
		const Node port_id = path_to_node_id((*p)->path());
		_model->add_statement(patch_id, "ingen:port", port_id);
		serialize_port(*p, port_id);
	}

	for (ConnectionList::const_iterator c = patch->connections().begin(); c != patch->connections().end(); ++c) {
		serialize_connection(*c);
	}
}


void
Serializer::serialize_plugin(SharedPtr<PluginModel> plugin)
{
	assert(_model);

	const Node plugin_id = Node(_model->world(), Node::RESOURCE, plugin->uri());

	_model->add_statement(
		plugin_id,
		"rdf:type",
		Node(_model->world(), Node::RESOURCE, plugin->type_uri()));
} 


void
Serializer::serialize_node(SharedPtr<NodeModel> node, const Node& node_id)
{
	const Node plugin_id = Node(_model->world(), Node::RESOURCE, node->plugin()->uri());

	_model->add_statement(
		node_id,
		"rdf:type",
		"ingen:Node");
	
	_model->add_statement(
		node_id,
		"ingen:name",
		node->path().name());
	
	_model->add_statement(
		node_id,
		"ingen:plugin",
		plugin_id);

	//serialize_plugin(node->plugin());
	
	/*_model->add_statement(_serializer,
		node_uri_ref.c_str(),
		"ingen:name",
		Atom(node->path().name()));*/

	for (PortModelList::const_iterator p = node->ports().begin(); p != node->ports().end(); ++p) {
		const Node port_id = path_to_node_id((*p)->path());
		serialize_port(*p, port_id);
		_model->add_statement(node_id, "ingen:port", port_id);
	}

	for (MetadataMap::const_iterator m = node->metadata().begin(); m != node->metadata().end(); ++m) {
		if (m->first.find(":") != string::npos) {
			_model->add_statement(
				node_id,
				m->first,
				m->second);
		}
	}
}

/** Writes a port subject with various information only if there are some
 * predicate/object pairs to go with it (eg if the port has metadata, or a value, or..).
 * Audio output ports with no metadata will not be written, for example.
 */
void
Serializer::serialize_port(SharedPtr<PortModel> port, const Node& port_id)
{
	if (port->is_input())
		_model->add_statement(port_id, "rdf:type", "ingen:InputPort");
	else
		_model->add_statement(port_id, "rdf:type", "ingen:OutputPort");

	_model->add_statement(port_id, "ingen:name", Atom(port->path().name().c_str()));
	
	_model->add_statement(port_id, "ingen:dataType", Atom(port->type()));
	
	if (port->is_control() && port->is_input())
		_model->add_statement(port_id, "ingen:value", Atom(port->value()));

	if (port->metadata().size() > 0) {
		for (MetadataMap::const_iterator m = port->metadata().begin(); m != port->metadata().end(); ++m) {
		if (m->first.find(":") != string::npos) {
				_model->add_statement(
						port_id,
						m->first,
						m->second);
			}
		}
	}
}


void
Serializer::serialize_connection(SharedPtr<ConnectionModel> connection) throw (std::logic_error)
{
	if (!_model)
		throw std::logic_error("serialize_connection called without serialization in progress");

	const Node connection_id = Node(_model->world(), Node::BLANK,
			path_to_node_id(connection->src_port_path()).to_string()
			+ "-" + path_to_node_id(connection->dst_port_path()).to_string());

	_model->add_statement(path_to_node_id(connection->dst_port_path()),
		"ingen:connectedTo",
		path_to_node_id(connection->src_port_path()));
	
	/*_model->add_statement(connection_id, "rdf:type", "ingen:Connection";
	
	_model->add_statement(connection_id,
		"ingen:source",
		path_to_node_id(connection->src_port_path()));
	
	_model->add_statement(connection_id,
		"ingen:destination",
		path_to_node_id(connection->dst_port_path()));
	*/
}


} // namespace Client
} // namespace Ingen
