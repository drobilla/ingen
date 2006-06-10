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


#ifndef NODECONTROLWINDOW_H
#define NODECONTROLWINDOW_H

#include <vector>
#include <string>
#include <gtkmm.h>
#include <libglademm.h>
#include <sigc++/sigc++.h>
#include "ControlPanel.h"
using std::string; using std::vector;

using namespace LibOmClient;

namespace OmGtk {
	
class ControlGroup;
class NodeController;


/** Window with controls (sliders) for all control-rate ports on a Node.
 *
 * \ingroup OmGtk
 */
class NodeControlWindow : public Gtk::Window
{
public:
	NodeControlWindow(NodeController* node, size_t poly);
	NodeControlWindow(NodeController* node, ControlPanel* panel);
	virtual ~NodeControlWindow();

	ControlPanel* control_panel() const { return m_control_panel; }
	
	void resize();

protected:
	void on_show();
	void on_hide();

private:
	NodeController*  m_node;
	ControlPanel*    m_control_panel;
	bool             m_callback_enabled;
	
	bool m_position_stored;
	int  m_x;
	int  m_y;
};


} // namespace OmGtk

#endif // NODECONTROLWINDOW_H
