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

#include <cassert>
#include <cmath>
#include "ingen-config.h"
#include "ingen/Port.hpp"
#include "shared/World.hpp"
#include "shared/LV2URIMap.hpp"
#include "NodeModel.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Client {

NodeModel::NodeModel(Shared::LV2URIMap&     uris,
                     SharedPtr<PluginModel> plugin,
                     const Path&            path)
	: Node()
	, ObjectModel(uris, path)
	, _plugin_uri(plugin->uri())
	, _plugin(plugin)
	, _num_values(0)
	, _min_values(0)
	, _max_values(0)
{
}

NodeModel::NodeModel(Shared::LV2URIMap& uris,
                     const URI&         plugin_uri,
                     const Path&        path)
	: Node()
	, ObjectModel(uris, path)
	, _plugin_uri(plugin_uri)
	, _num_values(0)
	, _min_values(0)
	, _max_values(0)
{
}

NodeModel::NodeModel(const NodeModel& copy)
	: Node(copy)
	, ObjectModel(copy)
	, _plugin_uri(copy._plugin_uri)
	, _num_values(copy._num_values)
	, _min_values((float*)malloc(sizeof(float) * _num_values))
	, _max_values((float*)malloc(sizeof(float) * _num_values))
{
	memcpy(_min_values, copy._min_values, sizeof(float) * _num_values);
	memcpy(_max_values, copy._max_values, sizeof(float) * _num_values);
}

NodeModel::~NodeModel()
{
	clear();
}

void
NodeModel::remove_port(SharedPtr<PortModel> port)
{
	for (Ports::iterator i = _ports.begin(); i != _ports.end(); ++i) {
		if ((*i) == port) {
			_ports.erase(i);
			break;
		}
	}
	_signal_removed_port.emit(port);
}

void
NodeModel::remove_port(const Path& port_path)
{
	for (Ports::iterator i = _ports.begin(); i != _ports.end(); ++i) {
		if ((*i)->path() == port_path) {
			_ports.erase(i);
			break;
		}
	}
}

void
NodeModel::clear()
{
	_ports.clear();
	assert(_ports.empty());
	delete[] _min_values;
	delete[] _max_values;
	_min_values = 0;
	_max_values = 0;
}

void
NodeModel::add_child(SharedPtr<ObjectModel> c)
{
	assert(c->parent().get() == this);

	//ObjectModel::add_child(c);

	SharedPtr<PortModel> pm = PtrCast<PortModel>(c);
	assert(pm);
	add_port(pm);
}

bool
NodeModel::remove_child(SharedPtr<ObjectModel> c)
{
	assert(c->path().is_child_of(path()));
	assert(c->parent().get() == this);

	//bool ret = ObjectModel::remove_child(c);

	SharedPtr<PortModel> pm = PtrCast<PortModel>(c);
	assert(pm);
	remove_port(pm);

	//return ret;
	return true;
}

void
NodeModel::add_port(SharedPtr<PortModel> pm)
{
	assert(pm);
	assert(pm->path().is_child_of(path()));
	assert(pm->parent().get() == this);

	Ports::iterator existing = find(_ports.begin(), _ports.end(), pm);

	// Store should have handled this by merging the two
	assert(existing == _ports.end());

	_ports.push_back(pm);
	_signal_new_port.emit(pm);
}

SharedPtr<const PortModel>
NodeModel::get_port(const Raul::Symbol& symbol) const
{
	for (Ports::const_iterator i = _ports.begin(); i != _ports.end(); ++i)
		if ((*i)->symbol() == symbol)
			return (*i);
	return SharedPtr<PortModel>();
}

Ingen::Port*
NodeModel::port(uint32_t index) const
{
	assert(index < num_ports());
	return const_cast<Ingen::Port*>(dynamic_cast<const Ingen::Port*>(_ports[index].get()));
}

void
NodeModel::default_port_value_range(SharedPtr<const PortModel> port,
                                    float& min, float& max) const
{
	// Default control values
	min = 0.0;
	max = 1.0;

	// Get range from client-side LV2 data
#ifdef HAVE_LILV
	if (_plugin && _plugin->type() == PluginModel::LV2) {

		if (!_min_values) {
			_num_values = lilv_plugin_get_num_ports(_plugin->lilv_plugin());
			_min_values = new float[_num_values];
			_max_values = new float[_num_values];
			lilv_plugin_get_port_ranges_float(_plugin->lilv_plugin(),
					_min_values, _max_values, 0);
		}

		if (!std::isnan(_min_values[port->index()]))
			min = _min_values[port->index()];
		if (!std::isnan(_max_values[port->index()]))
			max = _max_values[port->index()];
	}
#endif
}

void
NodeModel::port_value_range(SharedPtr<const PortModel> port, float& min, float& max) const
{
	assert(port->parent().get() == this);

	default_port_value_range(port, min, max);

	// Possibly overriden
	const Atom& min_atom = port->get_property(_uris.lv2_minimum);
	const Atom& max_atom = port->get_property(_uris.lv2_maximum);
	if (min_atom.type() == Atom::FLOAT)
		min = min_atom.get_float();
	if (max_atom.type() == Atom::FLOAT)
		max = max_atom.get_float();

	if (max <= min)
		max = min + 1.0;
}

std::string
NodeModel::port_label(SharedPtr<const PortModel> port) const
{
	const Raul::Atom& name = port->get_property("http://lv2plug.in/ns/lv2core#name");
	if (name.is_valid()) {
		return name.get_string();
	}

#ifdef HAVE_LILV
	if (_plugin && _plugin->type() == PluginModel::LV2) {
		LilvWorld*        c_world  = _plugin->lilv_world();
		const LilvPlugin* c_plugin = _plugin->lilv_plugin();
		LilvNode*         c_sym    = lilv_new_string(c_world, port->symbol().c_str());
		const LilvPort*   c_port   = lilv_plugin_get_port_by_symbol(c_plugin, c_sym);
		if (c_port) {
			LilvNode* c_name = lilv_port_get_name(c_plugin, c_port);
			if (c_name && lilv_node_is_string(c_name)) {
				std::string ret(lilv_node_as_string(c_name));
				lilv_node_free(c_name);
				return ret;
			}
			lilv_node_free(c_name);
		}
	}
#endif

	return port->symbol().c_str();
}

void
NodeModel::set(SharedPtr<ObjectModel> model)
{
	SharedPtr<NodeModel> node = PtrCast<NodeModel>(model);
	if (node) {
		_plugin_uri = node->_plugin_uri;
		_plugin     = node->_plugin;
	}

	ObjectModel::set(model);
}

} // namespace Client
} // namespace Ingen
