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


NodeModel::NodeModel(CountedPtr<PluginModel> plugin, const Path& path)
: ObjectModel(path),
  m_polyphonic(false),
  m_plugin_uri(plugin->uri()),
  m_plugin(plugin),
  m_x(0.0f),
  m_y(0.0f)
{
}

NodeModel::NodeModel(const string& plugin_uri, const Path& path)
: ObjectModel(path),
  m_polyphonic(false),
  m_plugin_uri(plugin_uri),
  m_x(0.0f),
  m_y(0.0f)
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
	const string old_path = m_path;
	
	ObjectModel::set_path(p);
	
	for (PortModelList::iterator i = m_ports.begin(); i != m_ports.end(); ++i)
		(*i)->set_path(m_path + "/" + (*i)->path().name());

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
	assert(c->path().is_child_of(m_path));
	assert(c->parent().get() == this);

	CountedPtr<PortModel> pm = PtrCast<PortModel>(c);
	assert(pm);
	remove_port(pm);
}


void
NodeModel::add_port(CountedPtr<PortModel> pm)
{
	assert(pm);
	assert(pm->path().is_child_of(m_path));
	assert(pm->parent().get() == this);

	PortModelList::iterator existing = m_ports.end();
	for (PortModelList::iterator i = m_ports.begin(); i != m_ports.end(); ++i) {
		if ((*i)->path() == pm->path()) {
			existing = i;
			break;
		}
	}

	if (existing != m_ports.end()) {
		cerr << "Warning: port clash, assimilating old port " << m_path << endl;
		pm->assimilate(*existing);
		*existing = pm;
	} else {
		m_ports.push_back(pm);
		new_port_sig.emit(pm);
	}
}


CountedPtr<PortModel>
NodeModel::get_port(const string& port_name)
{
	assert(port_name.find("/") == string::npos);
	for (PortModelList::iterator i = m_ports.begin(); i != m_ports.end(); ++i)
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


void
NodeModel::x(float a)
{
	if (m_x != a) {
		m_x = a;
		char temp_buf[16];
		snprintf(temp_buf, 16, "%f", a);
		set_metadata("module-x", temp_buf);
	}
}


void
NodeModel::y(float a)
{
	if (m_y != a) {
		m_y = a;
		char temp_buf[16];
		snprintf(temp_buf, 16, "%f", a);
		set_metadata("module-y", temp_buf);
	}
}


} // namespace Client
} // namespace Ingen
