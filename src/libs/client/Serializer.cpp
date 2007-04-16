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
#include <raptor.h>
#include "Serializer.h"
#include "PatchModel.h"
#include "NodeModel.h"
#include "ConnectionModel.h"
#include "PortModel.h"
#include "PresetModel.h"
#include "ModelEngineInterface.h"
#include "PluginModel.h"
#include "raul/Path.h"
#include "raul/Atom.h"
#include "raul/AtomRaptor.h"

using std::string; using std::vector; using std::pair;
using std::cerr; using std::cout; using std::endl;
using boost::optional;
using namespace Raul;

namespace Ingen {
namespace Client {


Serializer::Serializer()
{
	_writer.add_prefix("ingen", "http://drobilla.net/ns/ingen#");
	_writer.add_prefix("ingenuity", "http://drobilla.net/ns/ingenuity#");
	_writer.add_prefix("lv2", "http://lv2plug.in/ontology#");
	_writer.add_prefix("doap", "http://usefulinc.com/ns/doap#");
}


/** Begin a serialization to a file.
 *
 * This must be called before any serializing methods.
 */
void
Serializer::start_to_filename(const string& filename) throw (std::logic_error)
{
	_base_uri = "file://" + filename;
	_writer.start_to_filename(filename);
}


/** Begin a serialization to a string.
 *
 * This must be called before any serializing methods.
 *
 * The results of the serialization will be returned by the finish() method after
 * the desired objects have been serialized.
 */
void
Serializer::start_to_string() throw (std::logic_error)
{
	_base_uri = "";
	_writer.start_to_string();
}


/** Finish a serialization.
 *
 * If this was a serialization to a string, the serialization output
 * will be returned, otherwise the empty string is returned.
 */
string
Serializer::finish() throw(std::logic_error)
{
	return _writer.finish();
	_id_map.clear();
}


/** Convert a path to an RDF blank node ID for serializing.
 */
RdfId
Serializer::path_to_node_id(const Path& path)
{
	/*string ret = path.substr(1);

	for (size_t i=0; i < ret.length(); ++i) {
		if (ret[i] == '/')
			ret[i] = '_';
	}

	return RdfId(RdfId::ANONYMOUS, ret);
	*/

	IDMap::iterator i = _id_map.find(path);
	if (i != _id_map.end()) {
		return i->second;
	} else {
		const RdfId id = _writer.blank_id();
		_id_map[path] = id;
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
	if (!_writer.serialization_in_progress())
		throw std::logic_error("serialize called without serialization in progress");

	SharedPtr<PatchModel> patch = PtrCast<PatchModel>(object);
	if (patch) {
		serialize_patch(patch, RdfId(RdfId::RESOURCE, _base_uri));
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


void
Serializer::serialize_patch(SharedPtr<PatchModel> patch, const RdfId& patch_id)
{
	_writer.write(
		patch_id,
		NS_RDF("type"),
		NS_INGEN("Patch"));

	if (patch->path().name().length() > 0) {
		_writer.write(
			patch_id, NS_INGEN("name"),
			Atom(patch->path().name().c_str()));
	}

	_writer.write(
		patch_id,
		NS_INGEN("polyphony"),
		Atom((int)patch->poly()));
	
	for (MetadataMap::const_iterator m = patch->metadata().begin(); m != patch->metadata().end(); ++m) {
		if (_writer.expand_uri(m->first) != "") {
			_writer.write(
				patch_id,
				RdfId(RdfId::RESOURCE, _writer.expand_uri(m->first.c_str()).c_str()),
				m->second);
		}
	}

	for (NodeModelMap::const_iterator n = patch->nodes().begin(); n != patch->nodes().end(); ++n) {
		SharedPtr<PatchModel> patch = PtrCast<PatchModel>(n->second);
		if (patch) {
			const RdfId subpatch_id = RdfId(RdfId::RESOURCE,
					patch_id.to_string() + "#" + patch->path().substr(1));
			_writer.write(patch_id, NS_INGEN("node"), subpatch_id);
			serialize_patch(patch, subpatch_id);
		} else {
			const RdfId node_id = path_to_node_id(n->second->path());
			_writer.write(patch_id, NS_INGEN("node"), node_id);
			serialize_node(n->second, node_id);
		}
	}
	
	for (PortModelList::const_iterator p = patch->ports().begin(); p != patch->ports().end(); ++p) {
		const RdfId port_id = path_to_node_id((*p)->path());
		_writer.write(patch_id, NS_INGEN("port"), port_id);
		serialize_port(*p, port_id);
	}

	for (ConnectionList::const_iterator c = patch->connections().begin(); c != patch->connections().end(); ++c) {
		serialize_connection(*c);
	}
}


void
Serializer::serialize_plugin(SharedPtr<PluginModel> plugin)
{
	const RdfId plugin_id = RdfId(RdfId::RESOURCE, plugin->uri());

	_writer.write(
		plugin_id,
		NS_RDF("type"),
		RdfId(RdfId::RESOURCE, plugin->type_uri()));
}


void
Serializer::serialize_node(SharedPtr<NodeModel> node, const RdfId& node_id)
{
	const RdfId plugin_id = RdfId(RdfId::RESOURCE, node->plugin()->uri());

	_writer.write(
		node_id,
		NS_RDF("type"),
		NS_INGEN("Node"));
	
	_writer.write(
		node_id,
		NS_INGEN("name"),
		node->path().name());
	
	_writer.write(
		node_id,
		NS_INGEN("plugin"),
		plugin_id);

	//serialize_plugin(node->plugin());
	
	/*_writer.write(_serializer,
		node_uri_ref.c_str(),
		NS_INGEN("name"),
		Atom(node->path().name()));*/

	for (PortModelList::const_iterator p = node->ports().begin(); p != node->ports().end(); ++p) {
		const RdfId port_id = path_to_node_id((*p)->path());
		serialize_port(*p, port_id);
		_writer.write(node_id, NS_INGEN("port"), port_id);
	}

	for (MetadataMap::const_iterator m = node->metadata().begin(); m != node->metadata().end(); ++m) {
		if (_writer.expand_uri(m->first) != "") {
			_writer.write(
				node_id,
				RdfId(RdfId::RESOURCE, _writer.expand_uri(m->first.c_str()).c_str()),
				m->second);
		}
	}
}

/** Writes a port subject with various information only if there are some
 * predicate/object pairs to go with it (eg if the port has metadata, or a value, or..).
 * Audio output ports with no metadata will not be written, for example.
 */
void
Serializer::serialize_port(SharedPtr<PortModel> port, const RdfId& port_id)
{
	if (port->is_input())
		_writer.write(port_id, NS_RDF("type"), NS_INGEN("InputPort"));
	else
		_writer.write(port_id, NS_RDF("type"), NS_INGEN("OutputPort"));

	_writer.write(port_id, NS_INGEN("name"), Atom(port->path().name().c_str()));
	
	_writer.write(port_id, NS_INGEN("dataType"), Atom(port->type()));
	
	if (port->is_control() && port->is_input())
		_writer.write(port_id, NS_INGEN("value"), Atom(port->value()));

	if (port->metadata().size() > 0) {
		for (MetadataMap::const_iterator m = port->metadata().begin(); m != port->metadata().end(); ++m) {
			if (_writer.expand_uri(m->first) != "") {
				_writer.write(
						port_id,
						RdfId(RdfId::RESOURCE, _writer.expand_uri(m->first).c_str()),
						m->second);
			}
		}
	}
}


void
Serializer::serialize_connection(SharedPtr<ConnectionModel> connection) throw (std::logic_error)
{
	const RdfId connection_id = RdfId(RdfId::ANONYMOUS,
			path_to_node_id(connection->src_port_path()).to_string()
			+ "-" + path_to_node_id(connection->dst_port_path()).to_string());

	_writer.write(path_to_node_id(connection->dst_port_path()),
		NS_INGEN("connectedTo"),
		path_to_node_id(connection->src_port_path()));
	
	/*_writer.write(connection_id, NS_RDF("type"), NS_INGEN("Connection"));
	
	_writer.write(connection_id,
		NS_INGEN("source"),
		path_to_node_id(connection->src_port_path()));
	
	_writer.write(connection_id,
		NS_INGEN("destination"),
		path_to_node_id(connection->dst_port_path()));
	*/
}


} // namespace Client
} // namespace Ingen
