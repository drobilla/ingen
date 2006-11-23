/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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

#define U(x) ((const unsigned char*)(x))

using std::string; using std::vector; using std::pair;
using std::cerr; using std::cout; using std::endl;
using boost::optional;

namespace Ingen {
namespace Client {


Serializer::Serializer(SharedPtr<ModelEngineInterface> engine)
	: _serializer(NULL)
	, _string_output(NULL)
	, _patch_search_path(".")
	, _engine(engine)
{
	//_prefixes["xsd"]       = "http://www.w3.org/2001/XMLSchema#";
	_prefixes["rdf"]       = "http://www.w3.org/1999/02/22-rdf-syntax-ns#";
	_prefixes["ingen"]     = "http://codeson.net/ns/ingen#";
	_prefixes["ingenuity"] = "http://codeson.net/ns/ingenuity#";
}


Serializer::~Serializer()
{
}


/** Begin a serialization to a file.
 *
 * This must be called before any serializing methods.
 */
void
Serializer::start_to_filename(const string& filename) throw (std::logic_error)
{
	if (_serializer)
		throw std::logic_error("start_to_string called with serialization in progress");

	raptor_init();
	_serializer = raptor_new_serializer("rdfxml-abbrev");
	setup_prefixes();
	raptor_serialize_start_to_filename(_serializer, filename.c_str());
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
	if (_serializer)
		throw std::logic_error("start_to_string called with serialization in progress");

	raptor_init();
	_serializer = raptor_new_serializer("rdfxml-abbrev");
	setup_prefixes();
	raptor_serialize_start_to_string(_serializer,
	                                 NULL /*base_uri*/,
									 (void**)&_string_output,
									 NULL /*size*/);
}


/** Finish a serialization.
 *
 * If this was a serialization to a string, the serialization output
 * will be returned, otherwise the empty string is returned.
 */
string
Serializer::finish() throw(std::logic_error)
{
	string ret = "";

	if (!_serializer)
		throw std::logic_error("finish() called with no serialization in progress");

	raptor_serialize_end(_serializer);

	if (_string_output) {
		ret = string((char*)_string_output);
		free(_string_output);
		_string_output = NULL;
	}

	raptor_free_serializer(_serializer);
	_serializer = NULL;

	return ret;
}


void
Serializer::setup_prefixes()
{
	for (map<string,string>::const_iterator i = _prefixes.begin(); i != _prefixes.end(); ++i) {
		raptor_serialize_set_namespace(_serializer,
			raptor_new_uri(U(i->second.c_str())), U(i->first.c_str()));
	}
}


/** Expands the prefix of URI, if the prefix is registered.
 *
 * If uri is not a valid URI, the empty string is returned (so invalid URIs won't be serialized).
 */
string
Serializer::expand_uri(const string& uri)
{
	// FIXME: slow, stupid
	for (map<string,string>::const_iterator i = _prefixes.begin(); i != _prefixes.end(); ++i)
		if (uri.substr(0, i->first.length()+1) == i->first + ":")
			return i->second + uri.substr(i->first.length()+1);

	// FIXME: find a correct way to validate a URI
	if (uri.find(":") == string::npos && uri.find("/") == string::npos)
		return "";
	else
		return uri;
}


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


void
Serializer::serialize_resource(const string& subject_uri,
	                           const string& predicate_uri,
	                           const string& object_uri)
{
	assert(_serializer);

	raptor_statement triple;
	triple.subject = (void*)raptor_new_uri((const unsigned char*)subject_uri.c_str());
	triple.subject_type = RAPTOR_IDENTIFIER_TYPE_RESOURCE;
	triple.predicate = (void*)raptor_new_uri((const unsigned char*)predicate_uri.c_str());
	triple.predicate_type = RAPTOR_IDENTIFIER_TYPE_RESOURCE;
	triple.object = (void*)raptor_new_uri((const unsigned char*)object_uri.c_str());
	triple.object_type = RAPTOR_IDENTIFIER_TYPE_RESOURCE;
	
	raptor_serialize_statement(_serializer, &triple);
	
	raptor_free_uri((raptor_uri*)triple.subject);
	raptor_free_uri((raptor_uri*)triple.predicate);
}


void
Serializer::serialize_resource_blank(const string& node_id,
                                     const string& predicate_uri,
                                     const string& object_uri)
{
	assert(_serializer);

	raptor_statement triple;
	triple.subject = (const unsigned char*)node_id.c_str();
	triple.subject_type = RAPTOR_IDENTIFIER_TYPE_ANONYMOUS;
	triple.predicate = (void*)raptor_new_uri((const unsigned char*)predicate_uri.c_str());
	triple.predicate_type = RAPTOR_IDENTIFIER_TYPE_RESOURCE;
	triple.object = (void*)raptor_new_uri((const unsigned char*)object_uri.c_str());
	triple.object_type = RAPTOR_IDENTIFIER_TYPE_RESOURCE;
	
	raptor_serialize_statement(_serializer, &triple);
	
	raptor_free_uri((raptor_uri*)triple.predicate);
	raptor_free_uri((raptor_uri*)triple.object);
}

void
Serializer::serialize_atom(const string& subject_uri,
                           const string& predicate_uri,
                           const Atom&   atom)
{
	assert(_serializer);

	raptor_statement triple;
	triple.subject = (void*)raptor_new_uri((const unsigned char*)subject_uri.c_str());
	triple.subject_type = RAPTOR_IDENTIFIER_TYPE_RESOURCE;
	triple.predicate = (void*)raptor_new_uri((const unsigned char*)predicate_uri.c_str());
	triple.predicate_type = RAPTOR_IDENTIFIER_TYPE_RESOURCE;

	AtomRaptor::atom_to_triple_object(&triple, atom);

	raptor_serialize_statement(_serializer, &triple);
}


void
Serializer::serialize(SharedPtr<ObjectModel> object) throw (std::logic_error)
{
	if (!_serializer)
		throw std::logic_error("serialize_patch called without serialization in progress");

	SharedPtr<PatchModel> patch = PtrCast<PatchModel>(object);
	if (patch) {
		serialize_patch(patch);
		return;
	}
	
	SharedPtr<NodeModel> node = PtrCast<NodeModel>(object);
	if (node) {
		serialize_node(node);
		return;
	}
	
	SharedPtr<PortModel> port = PtrCast<PortModel>(object);
	if (port) {
		serialize_port(port);
		return;
	}

	cerr << "[Serializer] WARNING: Unsupported object type, "
		<< object->path() << " not serialized." << endl;
}


void
Serializer::serialize_patch(SharedPtr<PatchModel> patch)
{
	assert(_serializer);

	const string uri = "#";

	serialize_resource(
		uri.c_str(),
		NS_RDF("type"),
		NS_INGEN("Patch"));

	if (patch->path().name().length() > 0) {
		serialize_atom(
			uri.c_str(), NS_INGEN("name"),
			Atom(patch->path().name().c_str()));
	}

	serialize_atom(
		uri.c_str(),
		NS_INGEN("polyphony"),
		Atom((int)patch->poly()));

	for (NodeModelMap::const_iterator n = patch->nodes().begin(); n != patch->nodes().end(); ++n) {
		serialize_node(n->second, "#");
		serialize_resource("#", NS_INGEN("node"), uri + n->second->path().name());
	}
	
	for (PortModelList::const_iterator p = patch->ports().begin(); p != patch->ports().end(); ++p) {
		serialize_port(*p, uri);
		serialize_resource("#", NS_INGEN("port"), uri + (*p)->path().name());
	}

	for (ConnectionList::const_iterator c = patch->connections().begin(); c != patch->connections().end(); ++c) {
		serialize_connection(*c/*, uri*/);
	}
	
	//_engine->set_metadata(patch->path(), "uri", uri);
}


void
Serializer::serialize_node(SharedPtr<NodeModel> node, const string ns_prefix)
{
	assert(_serializer);

	const string node_uri = ns_prefix + node->path().name();

	serialize_resource(
		node_uri.c_str(),
		NS_RDF("type"),
		NS_INGEN("Node"));
	
	/*serialize_atom(_serializer,
		node_uri_ref.c_str(),
		NS_INGEN("name"),
		Atom(node->path().name()));*/

	for (PortModelList::const_iterator p = node->ports().begin(); p != node->ports().end(); ++p) {
		serialize_port(*p, node_uri + "/");
		serialize_resource(node_uri, NS_INGEN("port"), node_uri + "/" + (*p)->path().name());
	}

	for (MetadataMap::const_iterator m = node->metadata().begin(); m != node->metadata().end(); ++m) {
		if (expand_uri(m->first) != "") {
			serialize_atom(
				node_uri.c_str(),
				expand_uri(m->first.c_str()).c_str(),
				m->second);
		}
	}
}

/** Writes a port subject with various information only if there are some
 * predicate/object pairs to go with it (eg if the port has metadata, or a value, or..).
 * Audio output ports with no metadata will not be written, for example.
 */
void
Serializer::serialize_port(SharedPtr<PortModel> port, const string ns_prefix)
{
	assert(_serializer);

	const string port_uri_ref = ns_prefix + port->path().name();

	if (port->metadata().size() > 0) {

		serialize_resource(
				port_uri_ref.c_str(),
				NS_RDF("type"),
				NS_INGEN("Port"));

		for (MetadataMap::const_iterator m = port->metadata().begin(); m != port->metadata().end(); ++m) {
			if (expand_uri(m->first) != "") {
				serialize_atom(
						port_uri_ref.c_str(),
						expand_uri(m->first).c_str(),
						m->second);
			}
		}

	}
}


void
Serializer::serialize_connection(SharedPtr<ConnectionModel> connection) throw (std::logic_error)
{
	assert(_serializer);
	
	// (Double slash intentional)
	const string node_id = connection->src_port_path() + "/" + connection->dst_port_path();

	//raptor_identifier* c = raptor_new_identifier(RAPTOR_IDENTIFIER_TYPE_ANONYMOUS,
	//	NULL, RAPTOR_URI_SOURCE_BLANK_ID, (const unsigned char*)"genid1", NULL, NULL, NULL);

	const string src_port_rel_path = connection->src_port_path().substr(connection->patch_path().length());
	const string dst_port_rel_path = connection->dst_port_path().substr(connection->patch_path().length());

	serialize_resource_blank(node_id,
		NS_INGEN("source"),
		src_port_rel_path);
	
	serialize_resource_blank(node_id,
		NS_INGEN("destination"),
		dst_port_rel_path);
	
	serialize_resource_blank(node_id, NS_RDF("type"), NS_INGEN("Connection"));
}


/** Load a patch into the engine (e.g. from a patch file).
 *
 * @param base_uri Base URI (e.g. URI of the file to load from).
 *
 * @param data_path Path of the patch relative to base_uri
 * (e.g. relative to file root patch).
 * 
 * @param engine_path Path in the engine for the newly created patch
 * (i.e. 'destination' of the load).
 * 
 * @param initial_data will be set last, so values passed there will override
 * any values loaded from the patch file (or values in the existing patch).
 *
 * @param merge If true, the loaded patch's contents will be loaded into a
 * currently existing patch.  Errors may result if Nodes of conflicting names
 * (etc) exist - the loaded patch will take precendence (ie overwriting
 * existing values).  If false and the patch at @a load_path already exists,
 * load is aborted and false is returned.
 *
 * @param poly Polyphony of new loaded patch if given, otherwise polyphony
 * will be loaded (unless saved polyphony isn't found, in which case it's 1).
 *
 * Returns whether or not patch was loaded.
 */
bool
Serializer::load_patch(bool                    merge,
                       const string&           data_base_uri,
                       const Path&             data_path,
                       MetadataMap             engine_data,
                       optional<const Path&>   engine_parent,
                       optional<const string&> engine_name,
                       optional<size_t>        engine_poly)

{
	cerr << "LOAD: " << data_base_uri << ", " << data_path << ", " << engine_parent << endl;
#if 0
	cerr << "[Serializer] Loading patch " << filename << "" << endl;

	Path path = "/"; // path of the new patch

	const bool load_name = (name == "");
	const bool load_poly = (poly == 0);
	
	if (initial_data.find("filename") == initial_data.end())
		initial_data["filename"] = Atom(filename.c_str()); // FIXME: URL?

	xmlDocPtr doc = xmlParseFile(filename.c_str());

	if (!doc) {
		cerr << "Unable to parse patch file." << endl;
		return "";
	}

	xmlNodePtr cur = xmlDocGetRootElement(doc);

	if (!cur) {
		cerr << "Empty document." << endl;
		xmlFreeDoc(doc);
		return "";
	}

	if (xmlStrcmp(cur->name, (const xmlChar*) "patch")) {
		cerr << "File is not an Ingen patch file (root node != <patch>)" << endl;
		xmlFreeDoc(doc);
		return "";
	}

	xmlChar* key = NULL;
	cur = cur->xmlChildrenNode;

	// Load Patch attributes
	while (cur != NULL) {
		key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		
		if ((!xmlStrcmp(cur->name, (const xmlChar*)"name"))) {
			if (load_name) {
				assert(key != NULL);
				if (parent_path != "")
					path = Path(parent_path).base() + Path::nameify((char*)key);
			}
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"polyphony"))) {
			if (load_poly) {
				poly = atoi((char*)key);
			}
		} else if (xmlStrcmp(cur->name, (const xmlChar*)"connection")
				&& xmlStrcmp(cur->name, (const xmlChar*)"node")
				&& xmlStrcmp(cur->name, (const xmlChar*)"subpatch")
				&& xmlStrcmp(cur->name, (const xmlChar*)"filename")
				&& xmlStrcmp(cur->name, (const xmlChar*)"preset")) {
			// Don't know what this tag is, add it as metadata without overwriting
			// (so caller can set arbitrary parameters which will be preserved)
			if (key)
				if (initial_data.find((const char*)cur->name) == initial_data.end())
					initial_data[(const char*)cur->name] = (const char*)key;
		}
		
		xmlFree(key);
		key = NULL; // Avoid a (possible?) double free

		cur = cur->next;
	}
	
	if (poly == 0)
		poly = 1;

	// Create it, if we're not merging
	if (!existing)
		_engine->create_patch_with_data(path, poly, initial_data);

	// Load nodes
	cur = xmlDocGetRootElement(doc)->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar*)"node")))
			load_node(path, doc, cur);
			
		cur = cur->next;
	}

	// Load subpatches
	cur = xmlDocGetRootElement(doc)->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar*)"subpatch"))) {
			load_subpatch(path, doc, cur);
		}
		cur = cur->next;
	}
	
	// Load connections
	cur = xmlDocGetRootElement(doc)->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar*)"connection"))) {
			load_connection(path, doc, cur);
		}
		cur = cur->next;
	}
	
	
	// Load presets (control values)
	cerr << "FIXME: load preset\n";
	/*cur = xmlDocGetRootElement(doc)->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar*)"preset"))) {
			load_preset(pm, doc, cur);
			assert(preset_model != NULL);
			if (preset_model->name() == "default")
				_engine->set_preset(pm->path(), preset_model);
		}
		cur = cur->next;
	}
	*/
	
	xmlFreeDoc(doc);
	xmlCleanupParser();

	// Done above.. late enough?
	//_engine->set_metadata_map(path, initial_data);

	if (!existing)
		_engine->enable_patch(path);

	_load_path_translations.clear();

	return path;
#endif
	return "/FIXME";
}

} // namespace Client
} // namespace Ingen
