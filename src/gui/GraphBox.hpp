/*
  This file is part of Ingen.
  Copyright 2007-2016 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_GUI_GRAPH_BOX_HPP
#define INGEN_GUI_GRAPH_BOX_HPP

#include "ingen/ingen.h"

#include <gdk/gdk.h>
#include <glibmm/ustring.h>
#include <gtkmm/alignment.h>
#include <gtkmm/box.h>
#include <gtkmm/scrolledwindow.h>
#include <sigc++/connection.h>

#include <memory>
#include <string>

namespace Glib {
template <class T> class RefPtr;
} // namespace Glib

namespace Gtk {
class Builder;
class CheckMenuItem;
class HPaned;
class Label;
class MenuItem;
class Statusbar;
} // namespace Gtk

namespace Raul {
class Path;
} // namespace Raul

namespace ingen {

class Atom;
class URI;

namespace client {
class GraphModel;
class PortModel;
class ObjectModel;
} // namespace client

namespace gui {

class App;
class BreadCrumbs;
class GraphView;
class GraphWindow;

/** A window for a graph.
 *
 * \ingroup GUI
 */
class INGEN_API GraphBox : public Gtk::VBox
{
public:
	GraphBox(BaseObjectType*                   cobject,
	         const Glib::RefPtr<Gtk::Builder>& xml);

	~GraphBox() override;

	static std::shared_ptr<GraphBox>
	create(App& app, const std::shared_ptr<const client::GraphModel>& graph);

	void init_box(App& app);

	void set_status_text(const std::string& text);

	void set_graph(const std::shared_ptr<const client::GraphModel>& graph,
	               const std::shared_ptr<GraphView>&                view);

	void set_window(GraphWindow* win) { _window = win; }

	bool documentation_is_visible() { return _doc_scrolledwindow->is_visible(); }
	void set_documentation(const std::string& doc, bool html);

	std::shared_ptr<const client::GraphModel> graph() const { return _graph; }
	std::shared_ptr<GraphView>                view()  const { return _view; }

	void show_port_status(const client::PortModel* port,
	                      const Atom&              value);

	void set_graph_from_path(const Raul::Path&                 path,
	                         const std::shared_ptr<GraphView>& view);

	void object_entered(const client::ObjectModel* model);
	void object_left(const client::ObjectModel* model);

private:
	void graph_port_added(const std::shared_ptr<const client::PortModel>& port);
	void graph_port_removed(const std::shared_ptr<const client::PortModel>& port);
	void property_changed(const URI& predicate, const Atom& value);
	void show_status(const client::ObjectModel* model);

	void error(const Glib::ustring& message,
	           const Glib::ustring& secondary_text="");

	bool confirm(const Glib::ustring& message,
	             const Glib::ustring& secondary_text="");

	void save_graph(const URI& uri);

	void event_import();
	void event_save();
	void event_save_as();
	void event_export_image();
	void event_redo();
	void event_undo();
	void event_copy();
	void event_paste();
	void event_delete();
	void event_select_all();
	void event_close();
	void event_quit();
	void event_parent_activated();
	void event_refresh_activated();
	void event_fullscreen_toggled();
	void event_doc_pane_toggled();
	void event_status_bar_toggled();
	void event_animate_signals_toggled();
	void event_sprung_layout_toggled();
	void event_human_names_toggled();
	void event_port_names_toggled();
	void event_zoom_in();
	void event_zoom_out();
	void event_zoom_normal();
	void event_zoom_full();
	void event_increase_font_size();
	void event_decrease_font_size();
	void event_normal_font_size();
	void event_arrange();
	void event_show_properties();
	void event_show_engine();
	void event_clipboard_changed(GdkEventOwnerChange* ev);

	App*                                      _app = nullptr;
	std::shared_ptr<const client::GraphModel> _graph;
	std::shared_ptr<GraphView>                _view;
	GraphWindow*                              _window = nullptr;

	sigc::connection new_port_connection;
	sigc::connection removed_port_connection;
	sigc::connection edit_mode_connection;

	Gtk::MenuItem*      _menu_import                 = nullptr;
	Gtk::MenuItem*      _menu_save                   = nullptr;
	Gtk::MenuItem*      _menu_save_as                = nullptr;
	Gtk::MenuItem*      _menu_export_image           = nullptr;
	Gtk::MenuItem*      _menu_redo                   = nullptr;
	Gtk::MenuItem*      _menu_undo                   = nullptr;
	Gtk::MenuItem*      _menu_cut                    = nullptr;
	Gtk::MenuItem*      _menu_copy                   = nullptr;
	Gtk::MenuItem*      _menu_paste                  = nullptr;
	Gtk::MenuItem*      _menu_delete                 = nullptr;
	Gtk::MenuItem*      _menu_select_all             = nullptr;
	Gtk::MenuItem*      _menu_close                  = nullptr;
	Gtk::MenuItem*      _menu_quit                   = nullptr;
	Gtk::CheckMenuItem* _menu_animate_signals        = nullptr;
	Gtk::CheckMenuItem* _menu_sprung_layout          = nullptr;
	Gtk::CheckMenuItem* _menu_human_names            = nullptr;
	Gtk::CheckMenuItem* _menu_show_port_names        = nullptr;
	Gtk::CheckMenuItem* _menu_show_doc_pane          = nullptr;
	Gtk::CheckMenuItem* _menu_show_status_bar        = nullptr;
	Gtk::MenuItem*      _menu_zoom_in                = nullptr;
	Gtk::MenuItem*      _menu_zoom_out               = nullptr;
	Gtk::MenuItem*      _menu_zoom_normal            = nullptr;
	Gtk::MenuItem*      _menu_zoom_full              = nullptr;
	Gtk::MenuItem*      _menu_increase_font_size     = nullptr;
	Gtk::MenuItem*      _menu_decrease_font_size     = nullptr;
	Gtk::MenuItem*      _menu_normal_font_size       = nullptr;
	Gtk::MenuItem*      _menu_parent                 = nullptr;
	Gtk::MenuItem*      _menu_refresh                = nullptr;
	Gtk::MenuItem*      _menu_fullscreen             = nullptr;
	Gtk::MenuItem*      _menu_arrange                = nullptr;
	Gtk::MenuItem*      _menu_view_engine_window     = nullptr;
	Gtk::MenuItem*      _menu_view_control_window    = nullptr;
	Gtk::MenuItem*      _menu_view_graph_properties  = nullptr;
	Gtk::MenuItem*      _menu_view_messages_window   = nullptr;
	Gtk::MenuItem*      _menu_view_graph_tree_window = nullptr;
	Gtk::MenuItem*      _menu_help_about             = nullptr;

	Gtk::Alignment* _alignment    = nullptr;
	BreadCrumbs*    _breadcrumbs  = nullptr;
	Gtk::Statusbar* _status_bar   = nullptr;
	Gtk::Label*     _status_label = nullptr;

	Gtk::HPaned*         _doc_paned          = nullptr;
	Gtk::ScrolledWindow* _doc_scrolledwindow = nullptr;

	sigc::connection _entered_connection;
	sigc::connection _left_connection;

	/** Invisible bin used to store breadcrumbs when not shown by a view */
	Gtk::Alignment _breadcrumb_bin;

	bool _has_shown_documentation = false;
	bool _enable_signal           = true;
};

} // namespace gui
} // namespace ingen

#endif // INGEN_GUI_GRAPH_BOX_HPP
