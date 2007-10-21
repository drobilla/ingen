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

#include CONFIG_H_PATH

#include <cassert>
#include "interface/Port.hpp"
#include "NodeModel.hpp"
#include "PatchModel.hpp"

namespace Ingen {
namespace Client {


NodeModel::NodeModel(Store& store, SharedPtr<PluginModel> plugin, const Path& path, bool polyphonic)
	: ObjectModel(store, path, polyphonic)
	, _plugin_uri(plugin->uri())
	, _plugin(plugin)
{
}

NodeModel::NodeModel(Store& store, const string& plugin_uri, const Path& path, bool polyphonic)
	: ObjectModel(store, path, polyphonic)
	, _plugin_uri(plugin_uri)
{
}


NodeModel::~NodeModel()
{
	clear();
}


void
NodeModel::remove_port(SharedPtr<PortModel> port)
{
	// FIXME: slow
	for (PortModelList::iterator i = _ports.begin(); i != _ports.end(); ++i) {
		if ((*i) == port) {
			_ports.erase(i);
			break;
		}
	}
	signal_removed_port.emit(port);
}


void
NodeModel::remove_port(const Path& port_path)
{
	// FIXME: slow
	for (PortModelList::iterator i = _ports.begin(); i != _ports.end(); ++i) {
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
	assert(c->path().is_child_of(_path));
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
	assert(pm->path().is_child_of(_path));
	assert(pm->parent().get() == this);

	PortModelList::iterator existing = find(_ports.begin(), _ports.end(), pm);
	
	// Store should have handled this by merging the two
	assert(existing == _ports.end());

	_ports.push_back(pm);
	signal_new_port.emit(pm);
}


SharedPtr<PortModel>
NodeModel::get_port(const string& port_name) const
{
	assert(port_name.find("/") == string::npos);
	for (PortModelList::const_iterator i = _ports.begin(); i != _ports.end(); ++i)
		if ((*i)->path().name() == port_name)
			return (*i);
	return SharedPtr<PortModel>();
}
	

Shared::Port*
NodeModel::port(uint32_t index) const
{
	assert(index < num_ports());
	return dynamic_cast<Shared::Port*>(_ports[index].get());
}


void
NodeModel::port_value_range(SharedPtr<PortModel> port, float& min, float& max)
{
	assert(port->parent().get() == this);
	
#ifdef HAVE_SLV2
	// Plugin value first
	if (_plugin && _plugin->type() == PluginModel::LV2) {
		Glib::Mutex::Lock(PluginModel::rdf_world()->mutex());
		
		min = slv2_port_get_minimum_value(
				_plugin->slv2_plugin(),
				slv2_plugin_get_port_by_symbol(_plugin->slv2_plugin(),
					port->path().name().c_str()));
		
		max = slv2_port_get_maximum_value(
				_plugin->slv2_plugin(),
				slv2_plugin_get_port_by_symbol(_plugin->slv2_plugin(),
					port->path().name().c_str()));
	}
#endif

	// Possibly overriden
	const Atom& min_atom = port->get_variable("ingen:minimum");
	const Atom& max_atom = port->get_variable("ingen:maximum");
	if (min_atom.type() == Atom::FLOAT)
		min = min_atom.get_float();
	if (max_atom.type() == Atom::FLOAT)
		max = max_atom.get_float();

	cerr << port->path() << " VALUE RANGE: " << min << ", " << max << endl;
}



} // namespace Client
} // namespace Ingen
