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

#include <ctype.h>

#include <string>
#include <algorithm>

#include <boost/optional.hpp>

#include "raul/Path.hpp"

#include "ingen/Atom.hpp"
#include "ingen/client/GraphModel.hpp"
#include "ingen/client/PluginModel.hpp"
#include "ingen/client/PluginUI.hpp"

#include "ingen_config.h"

using namespace std;

namespace Ingen {
namespace Client {

LilvWorld*         PluginModel::_lilv_world   = NULL;
const LilvPlugins* PluginModel::_lilv_plugins = NULL;

Sord::World* PluginModel::_rdf_world = NULL;

PluginModel::PluginModel(URIs&                       uris,
                         const Raul::URI&            uri,
                         const Raul::URI&            type_uri,
                         const Resource::Properties& properties)
	: Plugin(uris, uri)
	, _type(type_from_uri(type_uri))
{
	if (_type == NIL) {
		if (uri.find("ingen-internals") != string::npos) {
			_type = Internal;
		} else {
			_type = LV2;  // Assume LV2 and hope Lilv can tell us something
		}
	}
	add_properties(properties);

	assert(_rdf_world);
	add_property(uris.rdf_type,
	             uris.forge.alloc_uri(this->type_uri()));
	LilvNode* plugin_uri = lilv_new_uri(_lilv_world, uri.c_str());
	_lilv_plugin = lilv_plugins_get_by_uri(_lilv_plugins, plugin_uri);
	lilv_node_free(plugin_uri);

	if (_type == Internal) {
		set_property(uris.doap_name,
		             uris.forge.alloc(uri.substr(uri.find_last_of('#') + 1)));
	}
}

static size_t
last_uri_delim(const std::string& str)
{
	for (size_t i = str.length() - 1; i > 0; --i) {
		switch (str[i]) {
		case ':': case '/': case '?': case '#':
			return i;
		}
	}
	return string::npos;
}

static bool
contains_alpha_after(const std::string& str, size_t begin)
{
	for (size_t i = begin; i < str.length(); ++i) {
		if (isalpha(str[i])) {
			return true;
		}
	}
	return false;
}

const Atom&
PluginModel::get_property(const Raul::URI& key) const
{
	static const Atom nil;
	const Atom& val = Resource::get_property(key);
	if (val.is_valid())
		return val;

	// No lv2:symbol from data or engine, invent one
	if (key == _uris.lv2_symbol) {
		string str        = this->uri();
		size_t last_delim = last_uri_delim(str);
		while (last_delim != string::npos &&
		       !contains_alpha_after(str, last_delim)) {
			str = str.substr(0, last_delim);
			last_delim = last_uri_delim(str);
		}
		str = str.substr(last_delim + 1);

		std::string symbol = Raul::Symbol::symbolify(str);
		set_property(_uris.lv2_symbol, _uris.forge.alloc(symbol));
		return get_property(key);
	}

	if (_lilv_plugin) {
		boost::optional<const Atom&> ret;
		LilvNode*  lv2_pred = lilv_new_uri(_lilv_world, key.c_str());
		LilvNodes* values   = lilv_plugin_get_value(_lilv_plugin, lv2_pred);
		lilv_node_free(lv2_pred);
		LILV_FOREACH(nodes, i, values) {
			const LilvNode* val = lilv_nodes_get(values, i);
			if (lilv_node_is_uri(val)) {
				ret = set_property(
					key, _uris.forge.alloc_uri(lilv_node_as_uri(val)));
				break;
			} else if (lilv_node_is_string(val)) {
				ret = set_property(
					key, _uris.forge.alloc(lilv_node_as_string(val)));
				break;
			} else if (lilv_node_is_float(val)) {
				ret = set_property(
					key, _uris.forge.make(lilv_node_as_float(val)));
				break;
			} else if (lilv_node_is_int(val)) {
				ret = set_property(
					key, _uris.forge.make(lilv_node_as_int(val)));
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
PluginModel::set(SPtr<PluginModel> p)
{
	_type = p->_type;

	_icon_path = p->_icon_path;
	if (p->_lilv_plugin)
		_lilv_plugin = p->_lilv_plugin;

	for (auto v : p->properties()) {
		Resource::set_property(v.first, v.second);
		_signal_property.emit(v.first, v.second);
	}

	_signal_changed.emit();
}

Raul::Symbol
PluginModel::default_block_symbol() const
{
	const Atom& name_atom = get_property(_uris.lv2_symbol);
	if (name_atom.is_valid() && name_atom.type() == _uris.forge.String)
		return Raul::Symbol::symbolify(name_atom.ptr<char>());
	else
		return Raul::Symbol("_");
}

string
PluginModel::human_name() const
{
	const Atom& name_atom = get_property(_uris.doap_name);
	if (name_atom.type() == _uris.forge.String)
		return name_atom.ptr<char>();
	else
		return default_block_symbol().c_str();
}

string
PluginModel::port_human_name(uint32_t i) const
{
	if (_lilv_plugin) {
		const LilvPort* port = lilv_plugin_get_port_by_index(_lilv_plugin, i);
		LilvNode*       name = lilv_port_get_name(_lilv_plugin, port);
		const string    ret(lilv_node_as_string(name));
		lilv_node_free(name);
		return ret;
	}
	return "";
}

PluginModel::ScalePoints
PluginModel::port_scale_points(uint32_t i) const
{
	// TODO: Non-float scale points
	ScalePoints points;
	if (_lilv_plugin) {
		const LilvPort*  port = lilv_plugin_get_port_by_index(_lilv_plugin, i);
		LilvScalePoints* sp   = lilv_port_get_scale_points(_lilv_plugin, port);
		LILV_FOREACH(scale_points, i, sp) {
			const LilvScalePoint* p = lilv_scale_points_get(sp, i);
			points.push_back(
				std::make_pair(
					lilv_node_as_float(lilv_scale_point_get_value(p)),
					lilv_node_as_string(lilv_scale_point_get_label(p))));
		}
	}
	return points;
}

bool
PluginModel::has_ui() const
{
	LilvUIs* uis = lilv_plugin_get_uis(_lilv_plugin);
	const bool ret = (lilv_nodes_size(uis) > 0);
	lilv_uis_free(uis);
	return ret;

}

SPtr<PluginUI>
PluginModel::ui(Ingen::World*          world,
                SPtr<const BlockModel> block) const
{
	if (!_lilv_plugin) {
		return SPtr<PluginUI>();
	}

	return PluginUI::create(world, block, _lilv_plugin);
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

static std::string
heading(const std::string& text, bool html, unsigned level)
{
	if (html) {
		const std::string tag = std::string("h") + std::to_string(level);
		return std::string("<") + tag + ">" + text + "</" + tag + ">\n";
	} else {
		return text + ":\n\n";
	}
}

static std::string
link(const std::string& addr, bool html)
{
	if (html) {
		return std::string("<a href=\"") + addr + "\">" + addr + "</a>";
	} else {
		return addr;
	}
}

std::string
PluginModel::get_documentation(const LilvNode* subject, bool html) const
{
	std::string doc;

	LilvNode* lv2_documentation = lilv_new_uri(_lilv_world,
	                                           LV2_CORE__documentation);
	LilvNode* rdfs_comment = lilv_new_uri(_lilv_world,
	                                      LILV_NS_RDFS "comment");

	LilvNodes* vals = lilv_world_find_nodes(
		_lilv_world, subject, lv2_documentation, NULL);
	const bool doc_is_html = vals;
	if (!vals) {
		vals = lilv_world_find_nodes(
			_lilv_world, subject, rdfs_comment, NULL);
	}

	if (vals) {
		const LilvNode* val = lilv_nodes_get_first(vals);
		if (lilv_node_is_string(val)) {
			doc += lilv_node_as_string(val);
		}
	}

	if (html && !doc_is_html) {
		for (std::size_t i = 0; i < doc.size(); ++i) {
			if (doc.substr(i, 2) == "\n\n") {
				doc.replace(i, 2, "<br/><br/>");
				i += strlen("<br/><br/>");
			}
		}
	}

	lilv_node_free(rdfs_comment);
	lilv_node_free(lv2_documentation);

	return doc;
}

std::string
PluginModel::documentation(const bool html) const
{
	LilvNode* subject = (_lilv_plugin)
		? lilv_node_duplicate(lilv_plugin_get_uri(_lilv_plugin))
		: lilv_new_uri(_lilv_world, uri().c_str());

	const std::string doc(get_documentation(subject, html));

	lilv_node_free(subject);

	return (heading(human_name(), html, 2) +
	        link(uri(), html) + (html ? "<br/><br/>" : "\n\n") +
	        doc);
}

std::string
PluginModel::port_documentation(uint32_t index, bool html) const
{
	if (!_lilv_plugin) {
		return "";
	}

	const LilvPort* port = lilv_plugin_get_port_by_index(_lilv_plugin, index);
	if (!port) {
		return "";
	}

	return (heading(port_human_name(index), html, 2) +
	        get_documentation(lilv_port_get_node(_lilv_plugin, port), html));
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
