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

#ifndef PATCHWINDOW_H
#define PATCHWINDOW_H

#include <string>
#include <list>
#include <gtkmm.h>
#include <libglademm/xml.h>
#include <libglademm.h>

using std::string; using std::list;

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
class PatchView;
class LoadPluginWindow;
class LoadPatchWindow;
class NewSubpatchWindow;
class LoadSubpatchWindow;
class NewSubpatchWindow;
class NodeControlWindow;
class PatchDescriptionWindow;
class SubpatchModule;
class OmPort;
class BreadCrumb;


/** A window for a patch.
 *
 * \ingroup OmGtk
 */
class PatchWindow : public Gtk::Window
{
public:
	PatchWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade_xml);
	~PatchWindow();
	
	void patch_controller(PatchController* pc);

	PatchController*    patch_controller() const     { return m_patch; }
	LoadPluginWindow*   load_plugin_window() const   { return m_load_plugin_window; }
	LoadSubpatchWindow* load_subpatch_window() const { return m_load_subpatch_window; }
	NewSubpatchWindow*  new_subpatch_window() const  { return m_new_subpatch_window; }

	// Breadcrumb management
	void node_removed(const string& name);
	void node_renamed(const string& old_path, const string& new_path);
	void patch_renamed(const string& new_path);
	void rebuild_breadcrumbs();
	void breadcrumb_clicked(BreadCrumb* crumb);
	
	Gtk::MenuItem* menu_view_control_window() { return m_menu_view_control_window; }

protected:
	void on_show();
	void on_hide();
	bool on_delete_event(GdkEventAny* ev);
	bool on_key_press_event(GdkEventKey* event);
	
private:
	//void event_open();
	void event_import();
	void event_save();
	void event_save_as();
	void event_close();
	void event_quit();
	void event_destroy();
	void event_clear();
	void event_fullscreen_toggled();
	void event_show_properties();
	void event_show_controls();
	void event_show_engine();

	PatchController*        m_patch;
	LoadPluginWindow*       m_load_plugin_window;
	LoadPatchWindow*        m_load_patch_window;
	NewSubpatchWindow*      m_new_subpatch_window;
	LoadSubpatchWindow*     m_load_subpatch_window;
	
	bool m_enable_signal;
	bool m_position_stored;
	int  m_x;
	int  m_y;
	
	//Gtk::MenuItem*      m_menu_open;
	Gtk::MenuItem*      m_menu_import;
	Gtk::MenuItem*      m_menu_save;
	Gtk::MenuItem*      m_menu_save_as;
	Gtk::MenuItem*      m_menu_configuration;
	Gtk::MenuItem*      m_menu_close;
	Gtk::MenuItem*      m_menu_quit;
	Gtk::CheckMenuItem* m_menu_fullscreen;
	Gtk::MenuItem*      m_menu_clear;
	Gtk::MenuItem*      m_menu_destroy_patch;
	Gtk::MenuItem*      m_menu_view_engine_window;
	Gtk::MenuItem*      m_menu_view_control_window;
	Gtk::MenuItem*      m_menu_view_patch_properties;
	Gtk::MenuItem*      m_menu_add_plugin;
	Gtk::MenuItem*      m_menu_new_subpatch;
	Gtk::MenuItem*      m_menu_load_subpatch;
	Gtk::MenuItem*      m_menu_view_messages_window;
	Gtk::MenuItem*      m_menu_view_patch_tree_window;
	Gtk::MenuItem*      m_menu_help_about;
	
	Gtk::VBox*          m_vbox;
	Gtk::Viewport*      m_viewport;
	Gtk::HBox*          m_breadcrumb_box;
	list<BreadCrumb*>   m_breadcrumbs;
	//Gtk::Statusbar*   m_status_bar;
};


} // namespace OmGtk

#endif // PATCHWINDOW_H
