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
 * You should have received a copy of the GNU General Public License alongCont
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "NodeControlWindow.h"
#include "GladeFactory.h"
#include "NodeModel.h"
#include "ControlGroups.h"
#include "ControlPanel.h"
#include "PatchWindow.h"
#include <iostream>
#include <cmath>
using std::cerr; using std::cout; using std::endl;

namespace Ingenuity {


/** Create a node control window and load a new ControlPanel for it.
 */
NodeControlWindow::NodeControlWindow(CountedPtr<NodeModel> node, size_t poly)
: m_node(node),
  m_position_stored(false),
  m_x(0), m_y(0)
{
	assert(m_node != NULL);
	
	property_resizable() = true;
	set_border_width(5);

	set_title(m_node->path() + " Controls");

	Glib::RefPtr<Gnome::Glade::Xml> xml = GladeFactory::new_glade_reference("warehouse_win");
	xml->get_widget_derived("control_panel_vbox", m_control_panel);
	m_control_panel->reparent(*this);
	
	m_control_panel->init(m_node, poly);
	
	show_all_children();
	resize();
	
	// FIXME: not working
	//set_icon_from_file(string(PKGDATADIR) + "/om-icon.png");

	m_callback_enabled = true;
}


/** Create a node control window and with an existing ControlPanel.
 */
NodeControlWindow::NodeControlWindow(CountedPtr<NodeModel> node, ControlPanel* panel)
: m_node(node),
  m_control_panel(panel)
{
	assert(m_node);
	
	property_resizable() = true;
	set_border_width(5);

	set_title(m_node->path() + " Controls");

	m_control_panel->reparent(*this);

	show_all_children();
	resize();
	
	m_callback_enabled = true;
}


NodeControlWindow::~NodeControlWindow()
{
	delete m_control_panel;
}


void
NodeControlWindow::resize()
{
	pair<int,int> controls_size = m_control_panel->ideal_size();
	/*int width = 400;
	int height = controls_size.second
		+ ((m_node->polyphonic()) ? 4 : 40);*/
	int width = controls_size.first;
	int height = controls_size.second;

	if (height > property_screen().get_value()->get_height() - 64)
		height = property_screen().get_value()->get_height() - 64;
	if (width > property_screen().get_value()->get_width() - 64)
		width = property_screen().get_value()->get_width() - 64;

	//cerr << "Resizing to: " << width << " x " << height << endl;

	Gtk::Window::resize(width, height);
}


void
NodeControlWindow::on_show()
{
	if (m_position_stored)
		move(m_x, m_y);

	Gtk::Window::on_show();
}


void
NodeControlWindow::on_hide()
{
	m_position_stored = true;
	get_position(m_x, m_y);
	Gtk::Window::on_hide();
}


} // namespace Ingenuity
