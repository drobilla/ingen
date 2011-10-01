/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
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

#ifndef INGEN_GUI_PATCHWINDOW_HPP
#define INGEN_GUI_PATCHWINDOW_HPP

#include <gtkmm.h>

#include "raul/SharedPtr.hpp"

#include "Window.hpp"

namespace Raul { class Atom; class Path; }

namespace Ingen {

namespace Client {
	class PatchModel;
	class PortModel;
	class ObjectModel;
}
using namespace Ingen::Client;

namespace GUI {

class BreadCrumbs;
class LoadPatchWindow;
class LoadPluginWindow;
class NewSubpatchWindow;
class NodeControlWindow;
class PatchDescriptionWindow;
class PatchView;
class SubpatchModule;

/** A window for a patch.
 *
 * \ingroup GUI
 */
class PatchWindow : public Window
{
public:
	PatchWindow(BaseObjectType*                   cobject,
	            const Glib::RefPtr<Gtk::Builder>& xml);
	~PatchWindow();

	void set_patch_from_path(const Raul::Path& path, SharedPtr<PatchView> view);
	void set_patch(SharedPtr<const PatchModel> pc, SharedPtr<PatchView> view);

	void show_documentation(const std::string& doc, bool html);
	void hide_documentation();

	SharedPtr<const PatchModel> patch() const { return _patch; }

	Gtk::MenuItem* menu_view_control_window() { return _menu_view_control_window; }

	void show_port_status(const PortModel* model, const Raul::Atom& value);

protected:
	void on_show();
	void on_hide();
	bool on_event(GdkEvent* event);

private:
	void patch_port_added(SharedPtr<const PortModel> port);
	void patch_port_removed(SharedPtr<const PortModel> port);
	void show_status(const ObjectModel* model);
	void object_entered(const ObjectModel* model);
	void object_left(const ObjectModel* model);
	void editable_changed(bool editable);

	void event_import();
	void event_import_location();
	void event_save();
	void event_save_as();
	void event_upload();
	void event_draw();
	void event_edit_controls();
	void event_copy();
	void event_paste();
	void event_delete();
	void event_select_all();
	void event_close();
	void event_quit();
	void event_fullscreen_toggled();
	void event_status_bar_toggled();
	void event_human_names_toggled();
	void event_port_names_toggled();
	void event_zoom_in();
	void event_zoom_out();
	void event_zoom_normal();
	void event_arrange();
	void event_show_properties();
	void event_show_controls();
	void event_show_engine();
	void event_clipboard_changed(GdkEventOwnerChange* ev);

	SharedPtr<const PatchModel> _patch;
	SharedPtr<PatchView>        _view;

	sigc::connection new_port_connection;
	sigc::connection removed_port_connection;
	sigc::connection edit_mode_connection;

	bool _enable_signal;
	bool _position_stored;
	int  _x;
	int  _y;

	Gtk::MenuItem*      _menu_import;
	Gtk::MenuItem*      _menu_import_location;
	Gtk::MenuItem*      _menu_save;
	Gtk::MenuItem*      _menu_save_as;
	Gtk::MenuItem*      _menu_upload;
	Gtk::MenuItem*      _menu_draw;
	Gtk::CheckMenuItem* _menu_edit_controls;
	Gtk::MenuItem*      _menu_cut;
	Gtk::MenuItem*      _menu_copy;
	Gtk::MenuItem*      _menu_paste;
	Gtk::MenuItem*      _menu_delete;
	Gtk::MenuItem*      _menu_select_all;
	Gtk::MenuItem*      _menu_close;
	Gtk::MenuItem*      _menu_quit;
	Gtk::CheckMenuItem* _menu_human_names;
	Gtk::CheckMenuItem* _menu_show_port_names;
	Gtk::CheckMenuItem* _menu_show_status_bar;
	Gtk::MenuItem*      _menu_zoom_in;
	Gtk::MenuItem*      _menu_zoom_out;
	Gtk::MenuItem*      _menu_zoom_normal;
	Gtk::MenuItem*      _menu_fullscreen;
	Gtk::MenuItem*      _menu_arrange;
	Gtk::MenuItem*      _menu_view_engine_window;
	Gtk::MenuItem*      _menu_view_control_window;
	Gtk::MenuItem*      _menu_view_patch_properties;
	Gtk::MenuItem*      _menu_view_messages_window;
	Gtk::MenuItem*      _menu_view_patch_tree_window;
	Gtk::MenuItem*      _menu_help_about;

	Gtk::VBox*          _vbox;
	Gtk::Alignment*     _alignment;
	BreadCrumbs*        _breadcrumbs;
	Gtk::Statusbar*     _status_bar;

	Gtk::HPaned*   _doc_paned;
	Gtk::Viewport* _doc_viewport;
	Gtk::TextView* _doc_textview;

	sigc::connection _entered_connection;
	sigc::connection _left_connection;

	/** Invisible bin used to store breadcrumbs when not shown by a view */
	Gtk::Alignment _breadcrumb_bin;

	bool _has_shown_documentation;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_PATCHWINDOW_HPP
