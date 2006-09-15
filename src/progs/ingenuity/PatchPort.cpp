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

#include "PatchPort.h"
#include <cassert>
#include <iostream>
#include "PortModel.h"
#include "PatchPortModule.h"
#include "ControlModel.h"
#include "Configuration.h"
#include "App.h"
using std::cerr; using std::endl;

using namespace Ingen::Client;

namespace Ingenuity {

PatchPort::PatchPort(PatchPortModule* module, CountedPtr<PortModel> pm)
: Port(module, pm->path().name(), !pm->is_input(), App::instance().configuration()->get_port_color(pm.get())),
  m_port_model(pm)
{
	assert(module);
	assert(m_port_model);
}

#if 0
void
PatchPort::set_name(const string& n)
{
	cerr << "********** PatchPort::set_name broken **********************" << endl;
	
	/* FIXME: move to PortController
	string new_path = Path::parent(m_port_model->path()) +"/"+ n;

	for (list<ControlPanel*>::iterator i = m_control_panels.begin(); i != m_control_panels.end(); ++i)
		(*i)->rename_port(m_port_model->path(), new_path);
	
	Port::set_name(n);
	m_port_model->path(new_path);
	*/
}
#endif

} // namespace Ingenuity
