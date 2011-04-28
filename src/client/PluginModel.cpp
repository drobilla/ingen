/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
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

#include <sstream>
#include <ctype.h>
#include <boost/optional.hpp>

#include "raul/Atom.hpp"
#include "raul/Path.hpp"

#include "PatchModel.hpp"
#include "PluginModel.hpp"
#include "PluginUI.hpp"
#include "shared/LV2URIMap.hpp"

#include "ingen-config.h"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Client {

#ifdef HAVE_LILV
LilvWorld   PluginModel::_lilv_world   = NULL;
LilvPlugins PluginModel::_lilv_plugins = NULL;
#endif

Sord::World* PluginModel::_rdf_world = NULL;

PluginModel::PluginModel(Shared::LV2URIMap& uris,
		const URI& uri, const URI& type_uri, const Resource::Properties& properties)
	: ResourceImpl(uris, uri)
	, _type(type_from_uri(type_uri.str()))
{
	add_properties(properties);

	assert(_rdf_world);
	add_property("http://www.w3.org/1999/02/22-rdf-syntax-ns#type", this->type_uri());
#ifdef HAVE_LILV
	LilvValue plugin_uri = lilv_value_new_uri(_lilv_world, uri.c_str());
	_lilv_plugin = lilv_plugins_get_by_uri(_lilv_plugins, plugin_uri);
	lilv_value_free(plugin_uri);
#endif
	if (_type == Internal)
		set_property("http://usefulinc.com/ns/doap#name",
				Atom(uri.substr(uri.find_last_of('#') + 1).c_str()));
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
		set_property("http://lv2plug.in/ns/lv2core#symbol", symbol);
		return get_property(key);
	}

#ifdef HAVE_LILV
	if (_lilv_plugin) {
		boost::optional<Raul::Atom&> ret;
		LilvValue  lv2_pred = lilv_value_new_uri(_lilv_world, key.str().c_str());
		LilvValues values   = lilv_plugin_get_value(_lilv_plugin, lv2_pred);
		lilv_value_free(lv2_pred);
		LILV_FOREACH(values, i, values) {
			LilvValue val = lilv_values_get(values, i);
			if (lilv_value_is_uri(val)) {
				ret = set_property(key, Atom(Atom::URI, lilv_value_as_uri(val)));
				break;
			} else if (lilv_value_is_string(val)) {
				ret = set_property(key, lilv_value_as_string(val));
				break;
			} else if (lilv_value_is_float(val)) {
				ret = set_property(key, Atom(lilv_value_as_float(val)));
				break;
			} else if (lilv_value_is_int(val)) {
				ret = set_property(key, Atom(lilv_value_as_int(val)));
				break;
			}
		}
		lilv_values_free(values);

		if (ret)
			return *ret;
	}
#endif

	return nil;
}

void
PluginModel::set(SharedPtr<PluginModel> p)
{
	_type = p->_type;

#ifdef HAVE_LILV
	_icon_path = p->_icon_path;
	if (p->_lilv_plugin)
		_lilv_plugin = p->_lilv_plugin;
#endif

	for (Properties::const_iterator v = p->properties().begin(); v != p->properties().end(); ++v) {
		ResourceImpl::set_property(v->first, v->second);
		signal_property.emit(v->first, v->second);
	}

	signal_changed.emit();
}

Symbol
PluginModel::default_node_symbol()
{
	const Atom& name_atom = get_property("http://lv2plug.in/ns/lv2core#symbol");
	if (name_atom.is_valid() && name_atom.type() == Atom::STRING)
		return Symbol::symbolify(name_atom.get_string());
	else
		return "_";
}

string
PluginModel::human_name()
{
	const Atom& name_atom = get_property("http://usefulinc.com/ns/doap#name");
	if (name_atom.type() == Atom::STRING)
		return name_atom.get_string();
	else
		return default_node_symbol().c_str();
}

string
PluginModel::port_human_name(uint32_t index) const
{
#ifdef HAVE_LILV
	if (_lilv_plugin) {
		LilvPort  port = lilv_plugin_get_port_by_index(_lilv_plugin, index);
		LilvValue name = lilv_port_get_name(_lilv_plugin, port);
		string    ret  = lilv_value_as_string(name);
		lilv_value_free(name);
		return ret;
	}
#endif
	return "";
}

#ifdef HAVE_LILV
bool
PluginModel::has_ui() const
{
	LilvUIs uis = lilv_plugin_get_uis(_lilv_plugin);
	return (lilv_values_size(uis) > 0);
}

SharedPtr<PluginUI>
PluginModel::ui(Ingen::Shared::World* world, SharedPtr<NodeModel> node) const
{
	if (_type != LV2)
		return SharedPtr<PluginUI>();

	SharedPtr<PluginUI> ret = PluginUI::create(world, node, _lilv_plugin);
	return ret;
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
PluginModel::get_lv2_icon_path(LilvPlugin plugin)
{
	string result;
	LilvValue svg_icon_pred = lilv_value_new_uri(_lilv_world,
		"http://ll-plugins.nongnu.org/lv2/namespace#svgIcon");

	LilvValues paths = lilv_plugin_get_value(plugin, svg_icon_pred);

	if (lilv_values_size(paths) > 0) {
		LilvValue value = lilv_values_get_first(paths);
		if (lilv_value_is_uri(value))
			result = lilv_uri_to_path(lilv_value_as_string(value));
		lilv_values_free(paths);
	}

	lilv_value_free(svg_icon_pred);
	return result;
}
#endif

std::string
PluginModel::documentation() const
{
	std::string doc;
	#ifdef HAVE_LILV
	if (!_lilv_plugin)
		return doc;

	//LilvValue lv2_documentation = lilv_value_new_uri(
	//	_lilv_world, LILV_NAMESPACE_LV2 "documentation");
	LilvValue rdfs_comment = lilv_value_new_uri(
		_lilv_world, "http://www.w3.org/2000/01/rdf-schema#comment");

	LilvValues vals = lilv_plugin_get_value(_lilv_plugin,
	                                        rdfs_comment);
	LilvValue val = lilv_values_get_first(vals);
	if (lilv_value_is_string(val)) {
		doc += lilv_value_as_string(val);
	}
	lilv_value_free(rdfs_comment);
	lilv_values_free(vals);
	#endif
	return doc;
}

std::string
PluginModel::port_documentation(uint32_t index) const
{
	std::string doc;
	#ifdef HAVE_LILV
	if (!_lilv_plugin)
		return doc;

	LilvPort  port = lilv_plugin_get_port_by_index(_lilv_plugin, index);

	//LilvValue lv2_documentation = lilv_value_new_uri(
	//	_lilv_world, LILV_NAMESPACE_LV2 "documentation");
	LilvValue rdfs_comment = lilv_value_new_uri(
		_lilv_world, "http://www.w3.org/2000/01/rdf-schema#comment");

	LilvValues vals = lilv_port_get_value(_lilv_plugin,
	                                      port,
	                                      rdfs_comment);
	LilvValue val = lilv_values_get_first(vals);
	if (lilv_value_is_string(val)) {
		doc += lilv_value_as_string(val);
	}
	lilv_value_free(rdfs_comment);
	lilv_values_free(vals);
	#endif
	return doc;
}

} // namespace Client
} // namespace Ingen
