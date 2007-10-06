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

#ifndef PORTMENU_H
#define PORTMENU_H

#include <string>
#include <gtkmm.h>
#include <raul/Path.hpp>
#include <raul/SharedPtr.hpp>
#include "client/PortModel.hpp"
#include "ObjectMenu.hpp"

using Ingen::Client::PortModel;

namespace Ingen {
namespace GUI {


/** Menu for a Port.
 *
 * \ingroup GUI
 */
class PortMenu : public ObjectMenu
{
public:
	PortMenu(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml);
	
	void init(SharedPtr<PortModel> port);
};


} // namespace GUI
} // namespace Ingen

#endif // PORTMENU_H
