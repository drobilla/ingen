/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
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
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef DSSICONTROLLER_H
#define DSSICONTROLLER_H

#include <string>
#include <gtkmm.h>
#include "util/Path.h"
#include "NodeController.h"

using std::string;
using Om::Path;
using namespace LibOmClient;

namespace LibOmClient {
	class MetadataModel;
	class NodeModel;
	class PortModel;
}

namespace OmGtk {

class Controller;
class OmModule;
class NodeControlWindow;
class NodePropertiesWindow;
class PortController;
class OmFlowCanvas;
class DSSIModule;

/* Controller for a DSSI node.
 *
 * \ingroup OmGtk
 */
class DSSIController : public NodeController
{
public:
	DSSIController(NodeModel* model);

	virtual ~DSSIController() {}
	
	void show_gui();
	bool attempt_to_show_gui();
	void program_add(int bank, int program, const string& name);
	void program_remove(int bank, int program);
	
	void send_program_change(int bank, int program);
	
	void create_module(OmFlowCanvas* canvas);

	void show_menu(GdkEventButton* event);

private:
	void update_program_menu();
	
	Gtk::Menu                   m_program_menu;
	Glib::RefPtr<Gtk::MenuItem> m_program_menu_item;
	
	bool m_banks_dirty;
};


} // namespace OmGtk

#endif // DSSICONTROLLER_H
