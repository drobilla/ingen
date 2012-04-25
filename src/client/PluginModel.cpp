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

#include <sstream>
#include <ctype.h>
#include <boost/optional.hpp>

#include "raul/Atom.hpp"
#include "raul/Path.hpp"

#include "ingen/shared/LV2URIMap.hpp"

#include "ingen/client/PatchModel.hpp"
#include "ingen/client/PluginModel.hpp"
#include "ingen/client/PluginUI.hpp"

#include "ingen_config.h"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Client {

LilvWorld*         PluginModel::_lilv_world   = NULL;
const LilvPlugins* PluginModel::_lilv_plugins = NULL;

Sord::World* PluginModel::_rdf_world = NULL;

PluginModel::PluginModel(Shared::URIs&               uris,
                         const URI&                  uri,
                         const URI&                  type_uri,
                         const Resource::Properties& properties)
	: ResourceImpl(uris, uri)
	, _type(type_from_uri(type_uri.str()))
{
	add_properties(properties);

	assert(_rdf_world);
	add_property("http://www.w3.org/1999/02/22-rdf-syntax-ns#type",
	             uris.forge.alloc_uri(this->type_uri().str()));
	LilvNode* plugin_uri = lilv_new_uri(_lilv_world, uri.c_str());
	_lilv_plugin = lilv_plugins_get_by_uri(_lilv_plugins, plugin_uri);
	lilv_node_free(plugin_uri);

	if (_type == Internal) {
		set_property("http://usefulinc.com/ns/doap#name",
		             uris.forge.alloc(uri.substr(uri.find_last_of('#') + 1)));
	}
}

const Atom&
PluginModel::get_property(const URI& key) const
{
	static const Atom nil;
	const Atom& val = ResourceImpl::get_property(key);
	if (val.is_valid())
		return val;

	// No lv2:symbol from data or engine, invent one
	if (key == _uris.lv2_symbol) {
		const URI& uri = this->uri();
		size_t last_slash = uri.find_last_of('/');
		size_t last_hash  = uri.find_last_of('#');
		string symbol;
		if (last_slash == string::npos && last_hash == string::npos) {
			size_t last_colon = uri.find_last_of(':');
			if (last_colon != string::npos)
				symbol = uri.substr(last_colon + 1);
			else
				symbol = uri.str();
		} else if (last_slash == string::npos) {
			symbol = uri.substr(last_hash + 1);
		} else if (last_hash == string::npos) {
			symbol = uri.substr(last_slash + 1);
		} else {
			size_t first_delim = std::min(last_slash, last_hash);
			size_t last_delim  = std::max(last_slash, last_hash);
			if (isalpha(uri.str()[last_delim + 1]))
				symbol = uri.str().substr(last_delim + 1);
			else
				symbol = uri.str().substr(first_delim + 1, last_delim - first_delim - 1);
		}
		set_property(LV2_CORE__symbol, _uris.forge.alloc(symbol));
		return get_property(key);
	}

	if (_lilv_plugin) {
		boost::optional<const Raul::Atom&> ret;
		LilvNode*  lv2_pred = lilv_new_uri(_lilv_world, key.str().c_str());
		LilvNodes* values   = lilv_plugin_get_value(_lilv_plugin, lv2_pred);
		lilv_node_free(lv2_pred);
		LILV_FOREACH(nodes, i, values) {
			const LilvNode* val = lilv_nodes_get(values, i);
			if (lilv_node_is_uri(val)) {
				ret = set_property(key, _uris.forge.alloc_uri(lilv_node_as_uri(val)));
				break;
			} else if (lilv_node_is_string(val)) {
				ret = set_property(key,
				                   _uris.forge.alloc(lilv_node_as_string(val)));
				break;
			} else if (lilv_node_is_float(val)) {
				ret = set_property(key,
				                   _uris.forge.make(lilv_node_as_float(val)));
				break;
			} else if (lilv_node_is_int(val)) {
				ret = set_property(key,
				                   _uris.forge.make(lilv_node_as_int(val)));
				break;
			}
		}
		lilv_nodes_free(values);

		if (ret)
			return *ret;
	}

	return nil;
}

void
PluginModel::set(SharedPtr<PluginModel> p)
{
	_type = p->_type;

	_icon_path = p->_icon_path;
	if (p->_lilv_plugin)
		_lilv_plugin = p->_lilv_plugin;

	for (Properties::const_iterator v = p->properties().begin(); v != p->properties().end(); ++v) {
		ResourceImpl::set_property(v->first, v->second);
		_signal_property.emit(v->first, v->second);
	}

	_signal_changed.emit();
}

Symbol
PluginModel::default_node_symbol() const
{
	const Atom& name_atom = get_property(LV2_CORE__symbol);
	if (name_atom.is_valid() && name_atom.type() == _uris.forge.String)
		return Symbol::symbolify(name_atom.get_string());
	else
		return "_";
}

string
PluginModel::human_name() const
{
	const Atom& name_atom = get_property("http://usefulinc.com/ns/doap#name");
	if (name_atom.type() == _uris.forge.String)
		return name_atom.get_string();
	else
		return default_node_symbol().c_str();
}

string
PluginModel::port_human_name(uint32_t index) const
{
	if (_lilv_plugin) {
		const LilvPort* port = lilv_plugin_get_port_by_index(_lilv_plugin, index);
		LilvNode*      name = lilv_port_get_name(_lilv_plugin, port);
		const string    ret(lilv_node_as_string(name));
		lilv_node_free(name);
		return ret;
	}
	return "";
}

bool
PluginModel::has_ui() const
{
	LilvUIs* uis = lilv_plugin_get_uis(_lilv_plugin);
	const bool ret = (lilv_nodes_size(uis) > 0);
	lilv_uis_free(uis);
	return ret;

}

SharedPtr<PluginUI>
PluginModel::ui(Ingen::Shared::World*      world,
                SharedPtr<const NodeModel> node) const
{
	if (!_lilv_plugin) {
		return SharedPtr<PluginUI>();
	}

	return PluginUI::create(world, node, _lilv_plugin);
}

const string&
PluginModel::icon_path() const
{
	if (_icon_path.empty() && _type == LV2) {
		_icon_path = get_lv2_icon_path(_lilv_plugin);
	}

	return _icon_path;
}

/** RDF world mutex must be held by the caller */
string
PluginModel::get_lv2_icon_path(const LilvPlugin* plugin)
{
	string result;
	LilvNode* svg_icon_pred = lilv_new_uri(_lilv_world,
		"http://ll-plugins.nongnu.org/lv2/namespace#svgIcon");

	LilvNodes* paths = lilv_plugin_get_value(plugin, svg_icon_pred);

	if (lilv_nodes_size(paths) > 0) {
		const LilvNode* value = lilv_nodes_get_first(paths);
		if (lilv_node_is_uri(value))
			result = lilv_uri_to_path(lilv_node_as_string(value));
		lilv_nodes_free(paths);
	}

	lilv_node_free(svg_icon_pred);
	return result;
}

std::string
PluginModel::documentation(bool* html) const
{
	std::string doc;
	if (!_lilv_plugin) {
		return doc;
	}

	LilvNode* lv2_documentation = lilv_new_uri(_lilv_world,
	                                           LILV_NS_LV2 "documentation");
	LilvNode* rdfs_comment = lilv_new_uri(_lilv_world,
	                                      LILV_NS_RDFS "comment");

	LilvNodes* vals = lilv_plugin_get_value(_lilv_plugin, lv2_documentation);
	if (vals) {
		*html = true;
		doc += std::string("<h2>") + human_name() + "</h2>\n";
	} else {
		*html = false;
		vals = lilv_plugin_get_value(_lilv_plugin, rdfs_comment);
	}

	if (vals) {
		const LilvNode* val = lilv_nodes_get_first(vals);
		if (lilv_node_is_string(val)) {
			doc += lilv_node_as_string(val);
		}
	}
	lilv_node_free(rdfs_comment);
	lilv_node_free(lv2_documentation);

	lilv_nodes_free(vals);

	return doc;
}

std::string
PluginModel::port_documentation(uint32_t index) const
{
	std::string doc;

	if (!_lilv_plugin)
		return doc;

	const LilvPort*  port = lilv_plugin_get_port_by_index(_lilv_plugin, index);

	//LilvNode lv2_documentation = lilv_new_uri(
	//	_lilv_world, LILV_NAMESPACE_LV2 "documentation");
	LilvNode* rdfs_comment = lilv_new_uri(
		_lilv_world, "http://www.w3.org/2000/01/rdf-schema#comment");

	LilvNodes* vals = lilv_port_get_value(_lilv_plugin,
	                                      port,
	                                      rdfs_comment);
	if (vals) {
		const LilvNode* val = lilv_nodes_get_first(vals);
		if (lilv_node_is_string(val)) {
			doc += lilv_node_as_string(val);
		}
	}
	lilv_node_free(rdfs_comment);
	lilv_nodes_free(vals);
	return doc;
}

const LilvPort*
PluginModel::lilv_port(uint32_t index) const
{
	return lilv_plugin_get_port_by_index(_lilv_plugin, index);
}

void
PluginModel::set_lilv_world(LilvWorld* world)
{
	_lilv_world   = world;
	_lilv_plugins = lilv_world_get_all_plugins(_lilv_world);
}

} // namespace Client
} // namespace Ingen
