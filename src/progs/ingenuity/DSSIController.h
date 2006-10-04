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

#ifndef DSSICONTROLLER_H
#define DSSICONTROLLER_H

#include <string>
#include <gtkmm.h>
#include "raul/Path.h"
#include "NodeModel.h"

using std::string;
using namespace Ingen::Client;

namespace Ingen { namespace Client {
	class MetadataModel;
	class NodeModel;
	class PortModel;
} }

namespace Ingenuity {

class NodeControlWindow;
class NodePropertiesWindow;

/* Controller for a DSSI node.
 *
 * FIXME: legacy cruft.  move this code to the appropriate places and nuke
 * this class.
 *
 * \ingroup Ingenuity
 */
class DSSIController
{
public:
	DSSIController(SharedPtr<NodeModel> model);

	virtual ~DSSIController() {}
	
	void show_gui();
	bool attempt_to_show_gui();
	void program_add(int bank, int program, const string& name);
	void program_remove(int bank, int program);
	
	void send_program_change(int bank, int program);
	
	void show_menu(GdkEventButton* event);

private:
	void update_program_menu();
	
	Gtk::Menu                   m_program_menu;
	Glib::RefPtr<Gtk::MenuItem> m_program_menu_item;
	
	bool m_banks_dirty;
};


} // namespace Ingenuity

#endif // DSSICONTROLLER_H
