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

#ifndef PATCHWINDOW_H
#define PATCHWINDOW_H

#include <string>
#include <list>
#include <gtkmm.h>
#include <libglademm/xml.h>
#include <libglademm.h>
#include "util/Path.h"
#include "util/CountedPtr.h"
#include "PatchModel.h"
#include "PatchView.h"
using Ingen::Client::PatchModel;

using std::string; using std::list;


namespace Ingen { namespace Client {
	class PatchModel;
	class PortModel;
	class ControlModel;
	class MetadataModel;
} }
using namespace Ingen::Client;


namespace Ingenuity {
	
class OmFlowCanvas;
class LoadPluginWindow;
class LoadPatchWindow;
class NewSubpatchWindow;
class LoadSubpatchWindow;
class NewSubpatchWindow;
class NodeControlWindow;
class PatchDescriptionWindow;
class SubpatchModule;
class OmPort;
class BreadCrumbBox;


/** A window for a patch.
 *
 * \ingroup Ingenuity
 */
class PatchWindow : public Gtk::Window
{
public:
	PatchWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade_xml);
	~PatchWindow();
	
	void set_patch_from_path(const Path& path, CountedPtr<PatchView> view);
	void set_patch(CountedPtr<PatchModel> pc, CountedPtr<PatchView> view);

	CountedPtr<PatchModel> patch()     const { return m_patch; }

	Gtk::MenuItem* menu_view_control_window() { return m_menu_view_control_window; }

	void claim_breadcrumbs();

protected:
	void on_show();
	void on_hide();
	bool on_key_press_event(GdkEventKey* event);
	
private:
	void event_import();
	void event_save();
	void event_save_as();
	void event_quit();
	void event_destroy();
	void event_clear();
	void event_fullscreen_toggled();
	void event_show_properties();
	void event_show_controls();
	void event_show_engine();

	CountedPtr<PatchModel> m_patch;
	CountedPtr<PatchView>  m_view;
	
	bool m_enable_signal;
	bool m_position_stored;
	int  m_x;
	int  m_y;
	
	Gtk::MenuItem*      m_menu_import;
	Gtk::MenuItem*      m_menu_save;
	Gtk::MenuItem*      m_menu_save_as;
	Gtk::MenuItem*      m_menu_configuration;
	Gtk::MenuItem*      m_menu_close;
	Gtk::MenuItem*      m_menu_quit;
	Gtk::MenuItem*      m_menu_fullscreen;
	Gtk::MenuItem*      m_menu_clear;
	Gtk::MenuItem*      m_menu_destroy_patch;
	Gtk::MenuItem*      m_menu_view_engine_window;
	Gtk::MenuItem*      m_menu_view_control_window;
	Gtk::MenuItem*      m_menu_view_patch_properties;
	Gtk::MenuItem*      m_menu_view_messages_window;
	Gtk::MenuItem*      m_menu_view_patch_tree_window;
	Gtk::MenuItem*      m_menu_help_about;
	
	Gtk::VBox*          m_vbox;
	Gtk::Viewport*      m_viewport;
	BreadCrumbBox*      m_breadcrumb_box;
	
	//Gtk::Statusbar*   m_status_bar;
	
	/** Invisible bin used to store breadcrumbs when not shown by a view */
	Gtk::Alignment m_breadcrumb_bin;
};


} // namespace Ingenuity

#endif // PATCHWINDOW_H
