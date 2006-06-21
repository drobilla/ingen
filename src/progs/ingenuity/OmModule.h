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
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */


#ifndef OMMODULE_H
#define OMMODULE_H

#include <string>
#include <libgnomecanvasmm.h>
#include <flowcanvas/Module.h>
#include "NodeController.h"
using std::string;

namespace LibOmClient {
class PortModel;
class NodeModel;
class ControlModel;
}
using namespace LibOmClient;

namespace OmGtk {
	
class PatchController;
class OmFlowCanvas;
class OmPort;


/** A module in a patch.
 *
 * This base class is extended for various types of modules - SubpatchModule,
 * DSSIModule, etc.
 *
 * \ingroup OmGtk
 */
class OmModule : public LibFlowCanvas::Module
{
public:
	OmModule(OmFlowCanvas* canvas, NodeController* node);
	virtual ~OmModule() {}
	
	virtual OmPort* port(const string& port_name) {
		return (OmPort*)Module::port(port_name);
	}

	virtual void store_location();
	void move_to(double x, double y);

	void on_right_click(GdkEventButton* event) { m_node->show_menu(event); }
	
	void show_control_window();

	NodeController* node() const { return m_node; }

protected:
	virtual void on_double_click(GdkEventButton* ev) { show_control_window(); }
	virtual void on_middle_click(GdkEventButton* ev) { show_control_window(); }
	
	NodeController* m_node;
};


} // namespace OmGtk

#endif // OMMODULE_H
