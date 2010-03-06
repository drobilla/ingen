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

#include <sstream>
#include <ctype.h>
#include <boost/optional.hpp>
#include "raul/Path.hpp"
#include "raul/Atom.hpp"
#include "ingen-config.h"
#include "shared/LV2URIMap.hpp"
#include "PluginModel.hpp"
#include "PatchModel.hpp"
#include "PluginUI.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Client {

#ifdef HAVE_SLV2
SLV2World   PluginModel::_slv2_world   = NULL;
SLV2Plugins PluginModel::_slv2_plugins = NULL;
#endif

Redland::World* PluginModel::_rdf_world = NULL;


PluginModel::PluginModel(Shared::LV2URIMap& uris,
		const URI& uri, const URI& type_uri, const Resource::Properties& properties)
	: ResourceImpl(uris, uri)
	, _type(type_from_uri(type_uri.str()))
{
	add_properties(properties);

	Glib::Mutex::Lock lock(_rdf_world->mutex());
	assert(_rdf_world);
	add_property("http://www.w3.org/1999/02/22-rdf-syntax-ns#type", this->type_uri());
#ifdef HAVE_SLV2
	SLV2Value plugin_uri = slv2_value_new_uri(_slv2_world, uri.c_str());
	_slv2_plugin = slv2_plugins_get_by_uri(_slv2_plugins, plugin_uri);
	slv2_value_free(plugin_uri);
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

#ifdef HAVE_SLV2
	if (_slv2_plugin) {
		boost::optional<Raul::Atom&> ret;
		SLV2Value  lv2_pred = slv2_value_new_uri(_slv2_world,
				_rdf_world->expand_uri(key.str()).c_str());
		SLV2Values values   = slv2_plugin_get_value(_slv2_plugin, lv2_pred);
		slv2_value_free(lv2_pred);
		for (unsigned i = 0; i < slv2_values_size(values); ++i) {
			SLV2Value val = slv2_values_get_at(values, 0);
			if (slv2_value_is_uri(val)) {
				ret = set_property(key, Atom(Atom::URI, slv2_value_as_uri(val)));
				break;
			} else if (slv2_value_is_string(val)) {
				ret = set_property(key, slv2_value_as_string(val));
				break;
			} else if (slv2_value_is_float(val)) {
				ret = set_property(key, Atom(slv2_value_as_float(val)));
				break;
			} else if (slv2_value_is_int(val)) {
				ret = set_property(key, Atom(slv2_value_as_int(val)));
				break;
			}
		}
		slv2_values_free(values);

		if (ret)
			return *ret;
	}
#endif

	return nil;
}


void
PluginModel::set(SharedPtr<PluginModel> p)
{
	_type      = p->_type;
	_icon_path = p->_icon_path;

#ifdef HAVE_SLV2
	if (p->_slv2_plugin)
		_slv2_plugin = p->_slv2_plugin;
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
#ifdef HAVE_SLV2
	if (_slv2_plugin) {
		Glib::Mutex::Lock lock(_rdf_world->mutex());
		SLV2Port  port = slv2_plugin_get_port_by_index(_slv2_plugin, index);
		SLV2Value name = slv2_port_get_name(_slv2_plugin, port);
		string    ret  = slv2_value_as_string(name);
		slv2_value_free(name);
		return ret;
	}
#endif
	return "";
}


#ifdef HAVE_SLV2
bool
PluginModel::has_ui() const
{
	Glib::Mutex::Lock lock(_rdf_world->mutex());

	SLV2Value gtk_gui_uri = slv2_value_new_uri(_slv2_world,
		"http://lv2plug.in/ns/extensions/ui#GtkUI");

	SLV2UIs uis = slv2_plugin_get_uis(_slv2_plugin);

	if (slv2_values_size(uis) > 0)
		for (unsigned i=0; i < slv2_uis_size(uis); ++i)
			if (slv2_ui_is_a(slv2_uis_get_at(uis, i), gtk_gui_uri))
				return true;

	return false;
}


SharedPtr<PluginUI>
PluginModel::ui(Ingen::Shared::World* world, SharedPtr<NodeModel> node) const
{
	if (_type != LV2)
		return SharedPtr<PluginUI>();

	SharedPtr<PluginUI> ret = PluginUI::create(world, node, _slv2_plugin);
	return ret;
}


const string&
PluginModel::icon_path() const
{
	if (_icon_path.empty() && _type == LV2) {
		Glib::Mutex::Lock lock(_rdf_world->mutex());
		_icon_path = get_lv2_icon_path(_slv2_plugin);
	}

	return _icon_path;
}


/** RDF world mutex must be held by the caller */
string
PluginModel::get_lv2_icon_path(SLV2Plugin plugin)
{
	string result;
	SLV2Value svg_icon_pred = slv2_value_new_uri(_slv2_world,
		"http://ll-plugins.nongnu.org/lv2/namespace#svgIcon");

	SLV2Values paths = slv2_plugin_get_value(plugin, svg_icon_pred);

	if (slv2_values_size(paths) > 0) {
		SLV2Value value = slv2_values_get_at(paths, 0);
		if (slv2_value_is_uri(value))
			result = slv2_uri_to_path(slv2_value_as_string(value));
		slv2_values_free(paths);
	}

	slv2_value_free(svg_icon_pred);
	return result;
}
#endif


} // namespace Client
} // namespace Ingen
