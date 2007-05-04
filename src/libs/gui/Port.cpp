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

#include <cassert>
#include <iostream>
#include "interface/EngineInterface.h"
#include "client/PatchModel.h"
#include "client/PortModel.h"
#include "client/ControlModel.h"
#include "Configuration.h"
#include "App.h"
#include "Port.h"
using std::cerr; using std::endl;

using namespace Ingen::Client;

namespace Ingen {
namespace GUI {


/** @param flip Make an input port appear as an output port, and vice versa.
 */
Port::Port(boost::shared_ptr<LibFlowCanvas::Module> module, SharedPtr<PortModel> pm, bool flip, bool destroyable)
: LibFlowCanvas::Port(module,
		pm->path().name(),
		flip ? (!pm->is_input()) : pm->is_input(),
		App::instance().configuration()->get_port_color(pm.get())),
  _port_model(pm)
{
	assert(module);
	assert(_port_model);

	if (destroyable)
		_menu.items().push_back(Gtk::Menu_Helpers::MenuElem("Destroy",
				sigc::mem_fun(this, &Port::on_menu_destroy)));
}


void
Port::on_menu_destroy()
{
	App::instance().engine()->destroy(_port_model->path());
}


} // namespace GUI
} // namespace Ingen
