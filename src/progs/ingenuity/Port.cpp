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

#include "Port.h"
#include <cassert>
#include <iostream>
#include "PortModel.h"
#include "NodeModule.h"
#include "ControlModel.h"
#include "Configuration.h"
#include "App.h"
using std::cerr; using std::endl;

using namespace Ingen::Client;

namespace Ingenuity {

Port::Port(NodeModule* module, CountedPtr<PortModel> pm)
: LibFlowCanvas::Port(module, pm->path().name(), pm->is_input(), App::instance().configuration()->get_port_color(pm.get())),
  m_port_model(pm)
{
	assert(module);
	assert(m_port_model);
}


} // namespace Ingenuity
