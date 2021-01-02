/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ingen/client/BlockModel.hpp"

#include "ingen/Atom.hpp"
#include "ingen/Forge.hpp"
#include "ingen/URIs.hpp"
#include "ingen/client/PluginModel.hpp"
#include "ingen/client/PortModel.hpp"
#include "lilv/lilv.h"
#include "lv2/core/lv2.h"
#include "raul/Path.hpp"
#include "raul/Symbol.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <utility>

namespace ingen {
namespace client {

BlockModel::BlockModel(URIs&                               uris,
                       const std::shared_ptr<PluginModel>& plugin,
                       const raul::Path&                   path)
    : ObjectModel(uris, path)
    , _plugin_uri(plugin->uri())
    , _plugin(plugin)
    , _num_values(0)
    , _min_values(nullptr)
    , _max_values(nullptr)
{
}

BlockModel::BlockModel(URIs& uris, URI plugin_uri, const raul::Path& path)
    : ObjectModel(uris, path)
    , _plugin_uri(std::move(plugin_uri))
    , _num_values(0)
    , _min_values(nullptr)
    , _max_values(nullptr)
{
}

BlockModel::BlockModel(const BlockModel& copy)
	: ObjectModel(copy)
	, _plugin_uri(copy._plugin_uri)
	, _num_values(copy._num_values)
	, _min_values(static_cast<float*>(malloc(sizeof(float) * _num_values)))
	, _max_values(static_cast<float*>(malloc(sizeof(float) * _num_values)))
{
	memcpy(_min_values, copy._min_values, sizeof(float) * _num_values);
	memcpy(_max_values, copy._max_values, sizeof(float) * _num_values);
}

BlockModel::~BlockModel()
{
	clear();
}

void
BlockModel::remove_port(const std::shared_ptr<PortModel>& port)
{
	for (auto i = _ports.begin(); i != _ports.end(); ++i) {
		if ((*i) == port) {
			_ports.erase(i);
			break;
		}
	}
	_signal_removed_port.emit(port);
}

void
BlockModel::remove_port(const raul::Path& port_path)
{
	for (auto i = _ports.begin(); i != _ports.end(); ++i) {
		if ((*i)->path() == port_path) {
			_ports.erase(i);
			break;
		}
	}
}

void
BlockModel::clear()
{
	_ports.clear();
	assert(_ports.empty());
	delete[] _min_values;
	delete[] _max_values;
	_min_values = nullptr;
	_max_values = nullptr;
}

void
BlockModel::add_child(const std::shared_ptr<ObjectModel>& c)
{
	assert(c->parent().get() == this);

	//ObjectModel::add_child(c);

	auto pm = std::dynamic_pointer_cast<PortModel>(c);
	assert(pm);
	add_port(pm);
}

bool
BlockModel::remove_child(const std::shared_ptr<ObjectModel>& c)
{
	assert(c->path().is_child_of(path()));
	assert(c->parent().get() == this);

	//bool ret = ObjectModel::remove_child(c);

	auto pm = std::dynamic_pointer_cast<PortModel>(c);
	assert(pm);
	remove_port(pm);

	//return ret;
	return true;
}

void
BlockModel::add_port(const std::shared_ptr<PortModel>& pm)
{
	assert(pm);
	assert(pm->path().is_child_of(path()));
	assert(pm->parent().get() == this);

	// Store should have handled this by merging the two
	assert(find(_ports.begin(), _ports.end(), pm) == _ports.end());

	_ports.push_back(pm);
	_signal_new_port.emit(pm);
}

std::shared_ptr<const PortModel>
BlockModel::get_port(const raul::Symbol& symbol) const
{
	for (auto p : _ports) {
		if (p->symbol() == symbol) {
			return p;
		}
	}
	return std::shared_ptr<PortModel>();
}

std::shared_ptr<const PortModel>
BlockModel::get_port(uint32_t index) const
{
	return _ports[index];
}

ingen::Node*
BlockModel::port(uint32_t index) const
{
	assert(index < num_ports());
	return const_cast<ingen::Node*>(
		dynamic_cast<const ingen::Node*>(_ports[index].get()));
}

void
BlockModel::default_port_value_range(
    const std::shared_ptr<const PortModel>& port,
    float&                                  min,
    float&                                  max,
    uint32_t                                srate) const
{
	// Default control values
	min = 0.0;
	max = 1.0;

	// Get range from client-side LV2 data
	if (_plugin && _plugin->lilv_plugin()) {
		if (!_min_values) {
			_num_values = lilv_plugin_get_num_ports(_plugin->lilv_plugin());
			_min_values = new float[_num_values];
			_max_values = new float[_num_values];
			lilv_plugin_get_port_ranges_float(_plugin->lilv_plugin(),
			                                  _min_values, _max_values, nullptr);
		}

		if (!std::isnan(_min_values[port->index()])) {
			min = _min_values[port->index()];
		}
		if (!std::isnan(_max_values[port->index()])) {
			max = _max_values[port->index()];
		}
	}

	if (port->port_property(_uris.lv2_sampleRate)) {
		min *= srate;
		max *= srate;
	}
}

void
BlockModel::port_value_range(const std::shared_ptr<const PortModel>& port,
                             float&                                  min,
                             float&                                  max,
                             uint32_t srate) const
{
	assert(port->parent().get() == this);

	default_port_value_range(port, min, max);

	// Possibly overriden
	const Atom& min_atom = port->get_property(_uris.lv2_minimum);
	const Atom& max_atom = port->get_property(_uris.lv2_maximum);
	if (min_atom.type() == _uris.forge.Float) {
		min = min_atom.get<float>();
	}
	if (max_atom.type() == _uris.forge.Float) {
		max = max_atom.get<float>();
	}

	if (max <= min) {
		max = min + 1.0f;
	}

	if (port->port_property(_uris.lv2_sampleRate)) {
		min *= srate;
		max *= srate;
	}
}

std::string
BlockModel::label() const
{
	const Atom& name_property = get_property(_uris.lv2_name);
	if (name_property.type() == _uris.forge.String) {
		return name_property.ptr<char>();
	} else if (plugin_model()) {
		return plugin_model()->human_name();
	} else {
		return symbol().c_str();
	}
}

std::string
BlockModel::port_label(const std::shared_ptr<const PortModel>& port) const
{
	const Atom& name = port->get_property(URI(LV2_CORE__name));
	if (name.is_valid() && name.type() == _uris.forge.String) {
		return name.ptr<char>();
	}

	if (_plugin && _plugin->lilv_plugin()) {
		LilvWorld*        w     = _plugin->lilv_world();
		const LilvPlugin* plug  = _plugin->lilv_plugin();
		LilvNode*         sym   = lilv_new_string(w, port->symbol().c_str());
		const LilvPort*   lport = lilv_plugin_get_port_by_symbol(plug, sym);
		if (lport) {
			LilvNode* lname = lilv_port_get_name(plug, lport);
			if (lname && lilv_node_is_string(lname)) {
				std::string ret(lilv_node_as_string(lname));
				lilv_node_free(lname);
				return ret;
			}
			lilv_node_free(lname);
		}
	}

	return port->symbol().c_str();
}

void
BlockModel::set(const std::shared_ptr<ObjectModel>& model)
{
	auto block = std::dynamic_pointer_cast<BlockModel>(model);
	if (block) {
		_plugin_uri = block->_plugin_uri;
		_plugin     = block->_plugin;
	}

	ObjectModel::set(model);
}

} // namespace client
} // namespace ingen
