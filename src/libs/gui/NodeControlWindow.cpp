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
 * You should have received a copy of the GNU General Public License alongCont
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <iostream>
#include <cmath>
#include "interface/EngineInterface.hpp"
#include "client/NodeModel.hpp"
#include "App.hpp"
#include "NodeControlWindow.hpp"
#include "GladeFactory.hpp"
#include "ControlGroups.hpp"
#include "ControlPanel.hpp"
#include "PatchWindow.hpp"

using namespace std;

namespace Ingen {
namespace GUI {


/** Create a node control window and load a new ControlPanel for it.
 */
NodeControlWindow::NodeControlWindow(SharedPtr<NodeModel> node, uint32_t poly)
: _node(node),
  _position_stored(false),
  _x(0), _y(0)
{
	assert(_node != NULL);
	
	property_resizable() = true;
	set_border_width(5);

	set_title(_node->path() + " Controls");

	Glib::RefPtr<Gnome::Glade::Xml> xml = GladeFactory::new_glade_reference("warehouse_win");
	xml->get_widget_derived("control_panel_vbox", _control_panel);
	
	_control_panel->reparent(*this);
	
	_control_panel->init(_node, poly);
	
	show_all_children();
	resize();
	
	_callback_enabled = true;
}


/** Create a node control window and with an existing ControlPanel.
 */
NodeControlWindow::NodeControlWindow(SharedPtr<NodeModel> node, ControlPanel* panel)
: _node(node),
  _control_panel(panel)
{
	assert(_node);
	
	property_resizable() = true;
	set_border_width(5);

	set_title(_node->path() + " Controls");

	_control_panel->reparent(*this);

	show_all_children();
	resize();
	
	_callback_enabled = true;
}


NodeControlWindow::~NodeControlWindow()
{
	delete _control_panel;
}


void
NodeControlWindow::resize()
{
	pair<int,int> controls_size = _control_panel->ideal_size();
	/*int width = 400;
	int height = controls_size.second
		+ ((_node->polyphonic()) ? 4 : 40);*/
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
	for (PortModelList::const_iterator i = _node->ports().begin();
			i != _node->ports().end(); ++i)
		if ((*i)->type().is_control() && (*i)->is_input())
			App::instance().engine()->request_port_value((*i)->path());

	if (_position_stored)
		move(_x, _y);

	Gtk::Window::on_show();
}


void
NodeControlWindow::on_hide()
{
	_position_stored = true;
	get_position(_x, _y);
	Gtk::Window::on_hide();
}


} // namespace GUI
} // namespace Ingen
