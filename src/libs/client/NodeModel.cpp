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

#include "NodeModel.h"
#include "PatchModel.h"
#include <cassert>

namespace Ingen {
namespace Client {


NodeModel::NodeModel(CountedPtr<PluginModel> plugin, const Path& path, bool polyphonic)
: ObjectModel(path),
  m_polyphonic(polyphonic),
  m_plugin_uri(plugin->uri()),
  m_plugin(plugin)
{
}

NodeModel::NodeModel(const string& plugin_uri, const Path& path, bool polyphonic)
: ObjectModel(path),
  m_polyphonic(polyphonic),
  m_plugin_uri(plugin_uri)
{
}


NodeModel::~NodeModel()
{
	clear();
}


void
NodeModel::remove_port(CountedPtr<PortModel> port)
{
	m_ports.remove(port);
	removed_port_sig.emit(port);
}


void
NodeModel::remove_port(const string& port_path)
{
	for (PortModelList::iterator i = m_ports.begin(); i != m_ports.end(); ++i) {
		if ((*i)->path() == port_path) {
			m_ports.erase(i);
			break;
		}
	}
}


void
NodeModel::clear()
{
	m_ports.clear();
	assert(m_ports.empty());
}


void
NodeModel::set_path(const Path& p)
{
	const string old_path = _path;
	
	ObjectModel::set_path(p);
	
	// FIXME: rename
//	for (PortModelList::iterator i = m_ports.begin(); i != m_ports.end(); ++i)
//		(*i)->set_path(_path + "/" + (*i)->path().name());

	//if (m_parent && old_path.length() > 0)
	//	parent_patch()->rename_node(old_path, p);
}


void
NodeModel::add_child(CountedPtr<ObjectModel> c)
{
	assert(c->parent().get() == this);

	CountedPtr<PortModel> pm = PtrCast<PortModel>(c);
	assert(pm);
	add_port(pm);
}


void
NodeModel::remove_child(CountedPtr<ObjectModel> c)
{
	assert(c->path().is_child_of(_path));
	assert(c->parent().get() == this);

	CountedPtr<PortModel> pm = PtrCast<PortModel>(c);
	assert(pm);
	remove_port(pm);
}


void
NodeModel::add_port(CountedPtr<PortModel> pm)
{
	assert(pm);
	assert(pm->path().is_child_of(_path));
	assert(pm->parent().get() == this);

	PortModelList::iterator existing = m_ports.end();
	for (PortModelList::iterator i = m_ports.begin(); i != m_ports.end(); ++i) {
		if ((*i)->path() == pm->path()) {
			existing = i;
			break;
		}
	}

	if (existing != m_ports.end()) {
		cerr << "Warning: port clash, assimilating old port " << _path << endl;
		pm->assimilate(*existing);
		*existing = pm;
	} else {
		m_ports.push_back(pm);
		new_port_sig.emit(pm);
	}
}


CountedPtr<PortModel>
NodeModel::get_port(const string& port_name) const
{
	assert(port_name.find("/") == string::npos);
	for (PortModelList::const_iterator i = m_ports.begin(); i != m_ports.end(); ++i)
		if ((*i)->path().name() == port_name)
			return (*i);
	return CountedPtr<PortModel>();
}


void 
NodeModel::add_program(int bank, int program, const string& name) 
{
        m_banks[bank][program] = name; 
}


void
NodeModel::remove_program(int bank, int program)
{
	m_banks[bank].erase(program);
	if (m_banks[bank].size() == 0)
		m_banks.erase(bank);
}


} // namespace Client
} // namespace Ingen
