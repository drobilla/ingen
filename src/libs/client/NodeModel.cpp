/* This file is part of Om.  Copyright (C) 2005 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
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

namespace LibOmClient {


NodeModel::NodeModel(CountedPtr<PluginModel> plugin, const Path& path)
: ObjectModel(path),
  m_polyphonic(false),
  m_plugin(plugin),
  m_x(0.0f),
  m_y(0.0f)
{
}

NodeModel::NodeModel(const Path& path)
: ObjectModel(path),
  m_polyphonic(false),
  m_plugin(NULL),
  m_x(0.0f),
  m_y(0.0f)
{
}


NodeModel::~NodeModel()
{
	/*for (PortModelList::iterator i = m_ports.begin(); i != m_ports.end(); ++i)
		delete(*i);*/
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
	/*for (PortModelList::iterator i = m_ports.begin(); i != m_ports.end(); ++i)
		delete (*i);*/

	m_ports.clear();

	assert(m_ports.empty());
}


void
NodeModel::set_path(const Path& p)
{
	const string old_path = m_path;
	
	ObjectModel::set_path(p);
	
	for (PortModelList::iterator i = m_ports.begin(); i != m_ports.end(); ++i)
		(*i)->set_path(m_path + "/" + (*i)->name());

	//if (m_parent && old_path.length() > 0)
	//	parent_patch()->rename_node(old_path, p);
}


void
NodeModel::add_port(CountedPtr<PortModel> pm)
{
	assert(pm);
	assert(pm->name() != "");
	assert(pm->path().length() > m_path.length());
	assert(pm->path().substr(0, m_path.length()) == m_path);
	assert(pm->parent().get() == this);
	assert(!get_port(pm->name()));

	m_ports.push_back(pm);

	new_port_sig.emit(pm);
}


CountedPtr<PortModel>
NodeModel::get_port(const string& port_name)
{
	assert(port_name.find("/") == string::npos);
	for (PortModelList::iterator i = m_ports.begin(); i != m_ports.end(); ++i)
		if ((*i)->name() == port_name)
			return (*i);
	return NULL;
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

}
