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

#ifndef OMPATCHBAYAREA_H
#define OMPATCHBAYAREA_H

#include <string>
#include <flowcanvas/FlowCanvas.h>


using std::string;
using namespace LibFlowCanvas;

using LibFlowCanvas::Port;

namespace OmGtk {
	
class OmModule;
class PatchController;

/** Patch canvas widget.
 *
 * \ingroup OmGtk
 */
class OmFlowCanvas : public LibFlowCanvas::FlowCanvas
{
public:
	OmFlowCanvas(PatchController* controller, int width, int height);
	
	OmModule* find_module(const string& name)
		{ return (OmModule*)FlowCanvas::find_module(name); }

	void connect(const Port* src_port, const Port* dst_port);
	void disconnect(const Port* src_port, const Port* dst_port);
	
	bool canvas_event(GdkEvent* event);
	void destroy_selected();

	void show_menu(GdkEvent* event)
	{ m_menu.popup(event->button.button, event->button.time); }
	
	void menu_load_plugin();
	void menu_load_subpatch();
	void menu_create_subpatch();

private:
	PatchController* m_patch_controller;
	int              m_last_click_x;
	int              m_last_click_y;
	
	Gtk::Menu m_menu;
};


} // namespace OmGtk

#endif // OMPATCHBAYAREA_H
