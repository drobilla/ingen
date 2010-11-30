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

#include <fstream>
#include <vector>
#include <algorithm>
#include <utility> // for pair, make_pair
#include <cassert>
#include <cstring>
#include <string>
#include <cstdlib>  // for atof
#include <cmath>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <glibmm/miscutils.h>
#include "raul/log.hpp"
#include "raul/Path.hpp"
#include "shared/LV2URIMap.hpp"
#include "interface/EngineInterface.hpp"
#include "DeprecatedLoader.hpp"

#define LOG(s) s << "[DeprecatedLoader] "

#define NS_INTERNALS "http://drobilla.net/ns/ingen-internals#"

using namespace std;
using namespace Raul;

namespace Ingen {
using namespace Shared;
namespace Client {


/** A single port's control setting (in a preset).
 *
 * \ingroup IngenClient
 */
class ControlModel
{
public:
	ControlModel(const Path& port_path, float value)
		: _port_path(port_path)
		, _value(value)
	{
		assert(_port_path.find("//") == string::npos);
	}

	const Path& port_path() const          { return _port_path; }
	void              port_path(const string& p) { _port_path = p; }
	float             value() const              { return _value; }
	void              value(float v)             { _value = v; }

private:
	Path _port_path;
	float      _value;
};


/** Model of a preset (a collection of control settings).
 *
 * \ingroup IngenClient
 */
class PresetModel
{
public:
	PresetModel(const string& base_path) : _base_path(base_path) {}

	/** Add a control value to this preset.  An empty string for a node_name
	 * means the port is on the patch itself (not a node in the patch). */
	void add_control(const string& node_name, string port_name, float value) {
		if (port_name == "note_number") // FIXME: filthy kludge
			port_name = "note";

		if (!node_name.empty())
			_controls.push_back(ControlModel(_base_path + node_name +"/"+ port_name, value));
		else
			_controls.push_back(ControlModel(_base_path + port_name, value));
	}

	const string& name() const          { return _name; }
	void          name(const string& n) { _name = n; }

	const list<ControlModel>& controls() const { return _controls; }

private:
	string             _name;
	string             _base_path;
	list<ControlModel> _controls;
};


string
DeprecatedLoader::nameify_if_invalid(const string& name)
{
	if (Path::is_valid_name(name)) {
		return name;
	} else {
		const string new_name = Path::nameify(name);
		assert(Path::is_valid_name(new_name));
		return new_name;
	}
}


string
DeprecatedLoader::translate_load_path(const string& path)
{
	std::map<string,string>::iterator t = _load_path_translations.find(path);

	if (t != _load_path_translations.end()) {
		assert(Path::is_valid((*t).second));
		return (*t).second;
	} else {
		return path;
	}
}


/** Add a piece of data to a Properties, translating from deprecated unqualified keys
 *
 * Adds a namespace prefix for known keys, and ignores the rest.
 */
void
DeprecatedLoader::add_variable(GraphObject::Properties& data, string old_key, string value)
{
	string key;
	if (old_key == "module-x")
		key = "http://drobilla.net/ns/ingenuity#canvas-x";
	else if (old_key == "module-y")
		key = "http://drobilla.net/ns/ingenuity#canvas-y";

	if (!key.empty()) {
		// FIXME: should this overwrite existing values?
		if (data.find(key) == data.end()) {
			// Hack to make module-x and module-y set as floats
			char* c_val = strdup(value.c_str());
			char* endptr = NULL;

			// FIXME: locale kludges
			char* locale = strdup(setlocale(LC_NUMERIC, NULL));

			float fval = strtof(c_val, &endptr);

			setlocale(LC_NUMERIC, locale);
			free(locale);

			if (endptr != c_val && *endptr == '\0')
				data.insert(make_pair(key, Atom(fval)));
			else
				data.insert(make_pair(key, value));

			free(c_val);
		}
	}
}


/** Load a patch in to the engine (and client) from a patch file.
 *
 * The name and poly from the passed PatchModel are used.  If the name is
 * the empty string, the name will be loaded from the file.  If the poly
 * is 0, it will be loaded from file.  Otherwise the given values will
 * be used.
 *
 * @param filename Local name of file to load patch from
 *
 * @param parent_path Patch to load this patch as a child of (empty string to load
 * to the root patch)
 *
 * @param name Name of this patch (loaded/generated if the empty string)
 *
 * @param initial_data will be set last, so values passed there will override
 * any values loaded from the patch file.
 *
 * @param existing If true, the patch will be loaded into a currently
 * existing patch (ie a merging will take place).  Errors will result
 * if Nodes of conflicting names exist.
 *
 * Returns the path of the newly created patch.
 */
string
DeprecatedLoader::load_patch(const Glib::ustring&    filename,
                             bool                    merge,
                             boost::optional<Path>   parent_path,
                             boost::optional<Symbol> name,
                             GraphObject::Properties initial_data,
                             bool                    existing)
{
	LOG(info) << "Loading patch " << filename << " under "
		<< parent_path << " / " << name << endl;

	Path path("/");
	if (parent_path)
		path = *parent_path;
	if (name)
		path = path.child(*name);

	size_t poly = 0;

	/* Use parameter overridden polyphony, if given */
	GraphObject::Properties::iterator poly_param = initial_data.find(_uris->ingen_polyphony);
	if (poly_param != initial_data.end() && poly_param->second.type() == Atom::INT)
		poly = poly_param->second.get_int32();

	if (initial_data.find(_uris->ingen_document) == initial_data.end())
		initial_data.insert(make_pair(_uris->ingen_document, filename));

	xmlDocPtr doc = xmlParseFile(filename.c_str());

	if (!doc) {
		LOG(error) << "Unable to parse patch file." << endl;
		return "";
	}

	xmlNodePtr cur = xmlDocGetRootElement(doc);

	if (!cur) {
		LOG(error) << "Empty document." << endl;
		xmlFreeDoc(doc);
		return "";
	}

	if (xmlStrcmp(cur->name, (const xmlChar*) "patch")) {
		LOG(error) << "File is not an Om patch file (root node != <patch>)" << endl;
		xmlFreeDoc(doc);
		return "";
	}

	xmlChar* key = NULL;
	cur = cur->xmlChildrenNode;

	// Load Patch attributes
	while (cur != NULL) {
		key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);

		if ((!xmlStrcmp(cur->name, (const xmlChar*)"name"))) {
			if (!merge && parent_path && !name && key) {
				if (parent_path)
					path = Path(parent_path.get()).base() + nameify_if_invalid((char*)key);
				else
					path = Path("/");
			}
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"polyphony"))) {
			if (poly == 0) {
				poly = atoi((char*)key);
			}
		} else if (xmlStrcmp(cur->name, (const xmlChar*)"connection")
				&& xmlStrcmp(cur->name, (const xmlChar*)"node")
				&& xmlStrcmp(cur->name, (const xmlChar*)"subpatch")
				&& xmlStrcmp(cur->name, (const xmlChar*)"filename")
				&& xmlStrcmp(cur->name, (const xmlChar*)"preset")) {
			// Don't know what this tag is, add it as variable without overwriting
			// (so caller can set arbitrary parameters which will be preserved)
			if (key)
				add_variable(initial_data, (const char*)cur->name, (const char*)key);
		}

		xmlFree(key);
		key = NULL; // Avoid a (possible?) double free

		cur = cur->next;
	}

	if (poly == 0)
		poly = 1;

	// Create it, if we're not merging
	if (!existing && !path.is_root()) {
		Resource::Properties props;
		props.insert(make_pair(_uris->rdf_type,        _uris->ingen_Patch));
		props.insert(make_pair(_uris->ingen_polyphony, Atom((int32_t)poly)));
		_engine->put(path, props);
		for (GraphObject::Properties::const_iterator i = initial_data.begin(); i != initial_data.end(); ++i)
			_engine->set_property(path, i->first, i->second);
	}

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
			load_subpatch(filename.substr(0, filename.find_last_of("/")), path, doc, cur);
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
	cur = xmlDocGetRootElement(doc)->xmlChildrenNode;
	while (cur != NULL) {
		// I don't think Om ever wrote any preset other than "default"...
		if ((!xmlStrcmp(cur->name, (const xmlChar*)"preset"))) {
			SharedPtr<PresetModel> pm = load_preset(path, doc, cur);
			assert(pm != NULL);
			if (pm->name() == "default") {
				list<ControlModel>::const_iterator i = pm->controls().begin();
				for ( ; i != pm->controls().end(); ++i) {
					const float value = i->value();
					_engine->set_property(translate_load_path(i->port_path().str()),
							_uris->ingen_value, Atom(value));
				}
			} else {
				LOG(warn) << "Unknown preset `" << pm->name() << "'" << endl;
			}
		}
		cur = cur->next;
	}

	xmlFreeDoc(doc);
	xmlCleanupParser();

	// Done above.. late enough?
	//for (Properties::const_iterator i = data.begin(); i != data.end(); ++i)
	//	_engine->set_property(subject, i->first, i->second);

	if (!existing)
		_engine->set_property(path, _uris->ingen_enabled, (bool)true);

	_load_path_translations.clear();

	return path.str();
}


bool
DeprecatedLoader::load_node(const Path& parent, xmlDocPtr doc, const xmlNodePtr node)
{
	xmlChar* key;
	xmlNodePtr cur = node->xmlChildrenNode;

	string path = "";
	bool   polyphonic = false;

	string plugin_uri;

	string plugin_type;  // deprecated
	string library_name; // deprecated
	string plugin_label; // deprecated

	GraphObject::Properties initial_data;

	while (cur != NULL) {
		key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);

		if ((!xmlStrcmp(cur->name, (const xmlChar*)"name"))) {
			path = parent.base() + nameify_if_invalid((char*)key);
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"polyphonic"))) {
			polyphonic = !strcmp((char*)key, "true");
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"type"))) {
			plugin_type = (const char*)key;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"library-name"))) {
			library_name = (char*)key;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"plugin-label"))) {
			plugin_label = (char*)key;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"plugin-uri"))) {
			plugin_uri = (char*)key;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"port"))) {
#if 0
			xmlNodePtr child = cur->xmlChildrenNode;

			string port_name;
			float user_min = 0.0;
			float user_max = 0.0;

			while (child != NULL) {
				key = xmlNodeListGetString(doc, child->xmlChildrenNode, 1);

				if ((!xmlStrcmp(child->name, (const xmlChar*)"name"))) {
					port_name = nameify_if_invalid((char*)key);
				} else if ((!xmlStrcmp(child->name, (const xmlChar*)"user-min"))) {
					user_min = atof((char*)key);
				} else if ((!xmlStrcmp(child->name, (const xmlChar*)"user-max"))) {
					user_max = atof((char*)key);
				}

				xmlFree(key);
				key = NULL; // Avoid a (possible?) double free

				child = child->next;
			}

			assert(path.length() > 0);
			assert(Path::is_valid(path));

			// FIXME: /nasty/ assumptions
			SharedPtr<PortModel> pm(new PortModel(Path(path).base() + port_name,
					PortModel::CONTROL, PortModel::INPUT, PortModel::NONE,
					0.0, user_min, user_max));
			//pm->set_parent(nm);
			nm->add_port(pm);
#endif

		} else {  // Don't know what this tag is, add it as variable
			if (key)
				add_variable(initial_data, (const char*)cur->name, (const char*)key);
		}
		xmlFree(key);
		key = NULL;

		cur = cur->next;
	}

	if (path.empty()) {
		LOG(error) << "Malformed patch file (node tag has empty children)" << endl;
		LOG(error) << "Node ignored." << endl;
		return false;
	}

	// Compatibility hacks for old patches that represent patch ports as nodes
	if (plugin_uri.empty()) {
		bool is_port = false;

		Resource::Properties props;

		if (plugin_type == "Internal") {
			is_port = true;
			if (plugin_label == "audio_input") {
				props.insert(make_pair(_uris->rdf_type, _uris->lv2_AudioPort));
				props.insert(make_pair(_uris->rdf_type, _uris->lv2_InputPort));
			} else if (plugin_label == "audio_output") {
				props.insert(make_pair(_uris->rdf_type, _uris->lv2_AudioPort));
				props.insert(make_pair(_uris->rdf_type, _uris->lv2_OutputPort));
			} else if (plugin_label == "control_input") {
				props.insert(make_pair(_uris->rdf_type, _uris->lv2_ControlPort));
				props.insert(make_pair(_uris->rdf_type, _uris->lv2_InputPort));
			} else if (plugin_label == "control_output" ) {
				props.insert(make_pair(_uris->rdf_type, _uris->lv2_ControlPort));
				props.insert(make_pair(_uris->rdf_type, _uris->lv2_OutputPort));
			} else if (plugin_label == "midi_input") {
				props.insert(make_pair(_uris->rdf_type, _uris->ev_EventPort));
				props.insert(make_pair(_uris->rdf_type, _uris->lv2_InputPort));
			} else if (plugin_label == "midi_output" ) {
				props.insert(make_pair(_uris->rdf_type, _uris->ev_EventPort));
				props.insert(make_pair(_uris->rdf_type, _uris->lv2_OutputPort));
			} else {
				is_port = false;
			}
		}

		if (is_port) {
			const string old_path = path;
			const string new_path = (Path::is_valid(old_path) ? old_path : Path::pathify(old_path));

			if (!Path::is_valid(old_path))
				LOG(warn) << "Translating invalid port path `" << old_path << "' => `"
					<< new_path << "'" << endl;

			// Set up translations (for connections etc) to alias both the old
			// module path and the old module/port path to the new port path
			if (old_path != new_path)
				_load_path_translations[old_path] = new_path;
			_load_path_translations[old_path + "/in"] = new_path;
			_load_path_translations[old_path + "/out"] = new_path;

			path = new_path;

			_engine->put(path, props);

			for (GraphObject::Properties::const_iterator i = initial_data.begin(); i != initial_data.end(); ++i)
				_engine->set_property(path, i->first, i->second);

			return false;

		} else {
			if (plugin_label == "note_in") {
				plugin_uri = NS_INTERNALS "Note";
				_load_path_translations[path + "/Frequency"] = path + "/frequency";
				_load_path_translations[path + "/Velocity"] = path + "/velocity";
				_load_path_translations[path + "/Gate"] = path + "/gate";
				_load_path_translations[path + "/Trigger"] = path + "/trigger";
			} else if (plugin_label == "control_input" || plugin_label == "midi_control_in") {
				plugin_uri = NS_INTERNALS "Controller";
				_load_path_translations[path + "/Controller_Number"] = path + "/controller";
				_load_path_translations[path + "/Logarithmic"] = path + "/logarithmic";
				_load_path_translations[path + "/Min"] = path + "/minimum";
				_load_path_translations[path + "/Max"] = path + "/maximum";
				_load_path_translations[path + "/Out"] = path + "/ar_output";
			} else if (plugin_label == "transport") {
				plugin_uri = NS_INTERNALS "Transport";
			} else if (plugin_label == "trigger_in") {
				plugin_uri = NS_INTERNALS "Trigger";
				_load_path_translations[path + "/Velocity"] = path + "/velocity";
				_load_path_translations[path + "/Gate"] = path + "/gate";
				_load_path_translations[path + "/Trigger"] = path + "/trigger";
			}

			if (plugin_uri.empty())
				plugin_uri = "om:" + plugin_type + ":" + library_name + ":" + plugin_label;

			Resource::Properties props;
			props.insert(make_pair(_uris->rdf_type,       _uris->ingen_Node));
			props.insert(make_pair(_uris->rdf_instanceOf, Atom(Atom::URI, plugin_uri)));
			_engine->put(path, props);

			_engine->set_property(path, _uris->ingen_polyphonic, bool(polyphonic));

			for (GraphObject::Properties::const_iterator i = initial_data.begin(); i != initial_data.end(); ++i)
				_engine->set_property(path, i->first, i->second);

			return true;
		}

	// Not deprecated
	} else {
		Resource::Properties props;
		props.insert(make_pair(_uris->rdf_type,       _uris->ingen_Node));
		props.insert(make_pair(_uris->rdf_instanceOf, Atom(Atom::URI, plugin_uri)));
		_engine->put(path, props);
		_engine->set_property(path, _uris->ingen_polyphonic, bool(polyphonic));
		for (GraphObject::Properties::const_iterator i = initial_data.begin(); i != initial_data.end(); ++i)
			_engine->set_property(path, i->first, i->second);
		return true;
	}

	// (shouldn't get here)
}


bool
DeprecatedLoader::load_subpatch(const string& base_filename, const Path& parent, xmlDocPtr doc, const xmlNodePtr subpatch)
{
	xmlChar *key;
	xmlNodePtr cur = subpatch->xmlChildrenNode;

	string name     = "";
	string filename = "";
	size_t poly     = 0;

	GraphObject::Properties initial_data;
	while (cur != NULL) {
		key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);

		if ((!xmlStrcmp(cur->name, (const xmlChar*)"name"))) {
			name = (const char*)key;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"polyphony"))) {
			initial_data.insert(make_pair(_uris->ingen_polyphony, (int)poly));
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"filename"))) {
			filename = Glib::build_filename(base_filename, (const char*)key);
		} else {  // Don't know what this tag is, add it as variable
			if (key != NULL && strlen((const char*)key) > 0)
				add_variable(initial_data, (const char*)cur->name, (const char*)key);
		}
		xmlFree(key);
		key = NULL;

		cur = cur->next;
	}

	LOG(info) << "Loading subpatch " << filename << " under " << parent << endl;
	// load_patch sets the passed variable last, so values stored in the parent
	// will override values stored in the child patch file
	load_patch(filename, false, parent, Symbol(nameify_if_invalid(name)), initial_data, false);

	return false;
}


bool
DeprecatedLoader::load_connection(const Path& parent, xmlDocPtr doc, const xmlNodePtr node)
{
	xmlChar *key;
	xmlNodePtr cur = node->xmlChildrenNode;

	string source_node, source_port, dest_node, dest_port;

	while (cur != NULL) {
		key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);

		if ((!xmlStrcmp(cur->name, (const xmlChar*)"source-node"))) {
			source_node = (char*)key;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"source-port"))) {
			source_port = (char*)key;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"destination-node"))) {
			dest_node = (char*)key;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"destination-port"))) {
			dest_port = (char*)key;
		}

		xmlFree(key);
		key = NULL; // Avoid a (possible?) double free

		cur = cur->next;
	}

	if (source_node.empty() || source_port.empty() || dest_node.empty() || dest_port.empty()) {
		LOG(error) << "Malformed patch file (connection tag has empty children)" << endl;
		LOG(error) << "Connection ignored." << endl;
		return false;
	}

	// Compatibility fixes for old (fundamentally broken) patches
	source_node = nameify_if_invalid(source_node);
	source_port = nameify_if_invalid(source_port);
	dest_node = nameify_if_invalid(dest_node);
	dest_port = nameify_if_invalid(dest_port);

	_engine->connect(
		translate_load_path(parent.base() + source_node +"/"+ source_port),
		translate_load_path(parent.base() + dest_node +"/"+ dest_port));

	return true;
}


/** Build a PresetModel given a pointer to a preset in a patch file. */
SharedPtr<PresetModel>
DeprecatedLoader::load_preset(const Path& parent, xmlDocPtr doc, const xmlNodePtr node)
{
	xmlNodePtr cur = node->xmlChildrenNode;
	xmlChar* key;

	SharedPtr<PresetModel> pm(new PresetModel(parent.base()));

	while (cur != NULL) {
		key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);

		if ((!xmlStrcmp(cur->name, (const xmlChar*)"name"))) {
			assert(key != NULL);
			pm->name((char*)key);
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"control"))) {
			xmlNodePtr child = cur->xmlChildrenNode;

			string node_name = "", port_name = "";
			float val = 0.0;

			while (child != NULL) {
				key = xmlNodeListGetString(doc, child->xmlChildrenNode, 1);

				if ((!xmlStrcmp(child->name, (const xmlChar*)"node-name"))) {
					node_name = (char*)key;
				} else if ((!xmlStrcmp(child->name, (const xmlChar*)"port-name"))) {
					port_name = (char*)key;
				} else if ((!xmlStrcmp(child->name, (const xmlChar*)"value"))) {
					val = atof((char*)key);
				}

				xmlFree(key);
				key = NULL; // Avoid a (possible?) double free

				child = child->next;
			}

			// Compatibility fixes for old patch files
			if (!node_name.empty())
				node_name = nameify_if_invalid(node_name);
			port_name = nameify_if_invalid(port_name);

			if (port_name.empty()) {
				string msg = "Unable to parse control in patch file ( node = ";
				msg.append(node_name).append(", port = ").append(port_name).append(")");
				LOG(error) << msg << endl;
			} else {
				// FIXME: temporary compatibility, remove any slashes from port name
				// remove this soon once patches have migrated
				string::size_type slash_index;
				while ((slash_index = port_name.find("/")) != string::npos)
					port_name[slash_index] = '-';

				pm->add_control(node_name, port_name, val);
			}
		}
		xmlFree(key);
		key = NULL;
		cur = cur->next;
	}
	if (pm->name().empty()) {
		LOG(error) << "Preset in patch file has no name." << endl;
		pm->name("Unnamed");
	}

	return pm;
}

} // namespace Client
} // namespace Ingen
