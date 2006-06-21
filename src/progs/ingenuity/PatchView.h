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

#ifndef PATCHVIEW_H
#define PATCHVIEW_H

#include <string>
#include <gtkmm.h>
#include <libglademm/xml.h>
#include <libglademm.h>

using std::string;

namespace LibOmClient {
class PatchModel;
class NodeModel;
class PortModel;
class ControlModel;
class MetadataModel;
}
using namespace LibOmClient;


namespace OmGtk {
	
class PatchController;
class OmFlowCanvas;
class LoadPluginWindow;
class NewSubpatchWindow;
class LoadSubpatchWindow;
class NewSubpatchWindow;
class NodeControlWindow;
class PatchDescriptionWindow;
class SubpatchModule;
class OmPort;


/** The patch specific contents of a PatchWindow (ie the canvas and whatever else).
 *
 * \ingroup OmGtk
 */
class PatchView : public Gtk::Box
{
public:
	PatchView(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade_xml);
	
	void patch_controller(PatchController* pc);

	OmFlowCanvas*    canvas() const           { return m_canvas; }
	PatchController* patch_controller() const { return m_patch; }

	void show_control_window();
	void zoom_changed();
	void process_toggled();

	void enabled(bool e);

private:
	PatchController*     m_patch;
	OmFlowCanvas*        m_canvas;
	
	Gtk::ScrolledWindow* m_canvas_scrolledwindow;
	Gtk::HScale*         m_zoom_slider;	
	Gtk::Label*          m_polyphony_label;
	Gtk::CheckButton*    m_process_checkbutton;
	
	bool m_enable_signal;
};


} // namespace OmGtk

#endif // PATCHVIEW_H
