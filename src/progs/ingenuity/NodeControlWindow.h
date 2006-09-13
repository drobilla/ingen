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

#ifndef NODECONTROLWINDOW_H
#define NODECONTROLWINDOW_H

#include <vector>
#include <string>
#include <gtkmm.h>
#include <libglademm.h>
#include <sigc++/sigc++.h>
#include "util/CountedPtr.h"
using std::string; using std::vector;

namespace Ingen { namespace Client {
	class NodeModel;
} }
using Ingen::Client::NodeModel;

namespace Ingenuity {
	
class ControlGroup;
class ControlPanel;


/** Window with controls (sliders) for all control-rate ports on a Node.
 *
 * \ingroup Ingenuity
 */
class NodeControlWindow : public Gtk::Window
{
public:
	NodeControlWindow(CountedPtr<NodeModel> node, size_t poly);
	NodeControlWindow(CountedPtr<NodeModel> node, ControlPanel* panel);
	virtual ~NodeControlWindow();

	CountedPtr<NodeModel> node() { return m_node; }

	ControlPanel* control_panel() const { return m_control_panel; }
	
	void resize();

protected:
	void on_show();
	void on_hide();

private:
	CountedPtr<NodeModel>    m_node;
	ControlPanel* m_control_panel;
	bool          m_callback_enabled;
	
	bool m_position_stored;
	int  m_x;
	int  m_y;
};


} // namespace Ingenuity

#endif // NODECONTROLWINDOW_H
