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

#include <iostream>
#include <gtkmm.h>
#include <raul/SharedPtr.hpp>
#include "interface/EngineInterface.hpp"
#include "client/PortModel.hpp"
#include "App.hpp"
#include "PortMenu.hpp"
#include "WindowFactory.hpp"

namespace Ingen {
namespace GUI {


PortMenu::PortMenu(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
	: ObjectMenu(cobject, xml)
{
}


void
PortMenu::init(SharedPtr<PortModel> port)
{
	ObjectMenu::init(port);
	
	if ( ! PtrCast<PatchModel>(port->parent()) ) {
		_polyphonic_menuitem->set_sensitive(false);
		_rename_menuitem->hide();
		_destroy_menuitem->hide();
	}
		
	_enable_signal = true;
}


} // namespace GUI
} // namespace Ingen

