/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_GUI_GRAPHTREEWINDOW_HPP
#define INGEN_GUI_GRAPHTREEWINDOW_HPP

#include "Window.hpp"

#include <gtkmm/builder.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/treestore.h>
#include <gtkmm/treeview.h>

namespace Raul { class Path; }

namespace ingen {

namespace client { class ClientStore; class ObjectModel; }

namespace gui {

class GraphWindow;
class GraphTreeView;

/** Window with a TreeView of all loaded graphs.
 *
 * \ingroup GUI
 */
class GraphTreeWindow : public Window
{
public:
	GraphTreeWindow(BaseObjectType*                   cobject,
	                const Glib::RefPtr<Gtk::Builder>& xml);

	void init(App& app, client::ClientStore& store);

	void new_object(const SPtr<client::ObjectModel>& object);

	void graph_property_changed(const URI&                      key,
	                            const Atom&                     value,
	                            const SPtr<client::GraphModel>& graph);

	void graph_moved(const SPtr<client::GraphModel>& graph);

	void add_graph(const SPtr<client::GraphModel>& pm);
	void remove_graph(const SPtr<client::GraphModel>& pm);
	void show_graph_menu(GdkEventButton* ev);

protected:
	void event_graph_activated(const Gtk::TreeModel::Path& path,
	                           Gtk::TreeView::Column*      col);

	void event_graph_enabled_toggled(const Glib::ustring& path_str);

	Gtk::TreeModel::iterator find_graph(
		Gtk::TreeModel::Children  root,
		const SPtr<client::ObjectModel>& graph);

	GraphTreeView* _graphs_treeview;

	struct GraphTreeModelColumns : public Gtk::TreeModel::ColumnRecord
	{
		GraphTreeModelColumns() {
			add(name_col);
			add(enabled_col);
			add(graph_model_col);
		}

		Gtk::TreeModelColumn<Glib::ustring>             name_col;
		Gtk::TreeModelColumn<bool>                      enabled_col;
		Gtk::TreeModelColumn<SPtr<client::GraphModel> > graph_model_col;
	};

	App*                             _app;
	GraphTreeModelColumns            _graph_tree_columns;
	Glib::RefPtr<Gtk::TreeStore>     _graph_treestore;
	Glib::RefPtr<Gtk::TreeSelection> _graph_tree_selection;
	bool                             _enable_signal;
};

/** Derived TreeView class to support context menus for graphs */
class GraphTreeView : public Gtk::TreeView
{
public:
	GraphTreeView(BaseObjectType*                   cobject,
	              const Glib::RefPtr<Gtk::Builder>& xml)
		: Gtk::TreeView(cobject)
		, _window(nullptr)
	{}

	void set_window(GraphTreeWindow* win) { _window = win; }

	bool on_button_press_event(GdkEventButton* ev) override {
		bool ret = Gtk::TreeView::on_button_press_event(ev);

		if ((ev->type == GDK_BUTTON_PRESS) && (ev->button == 3)) {
			_window->show_graph_menu(ev);
		}

		return ret;
	}

private:
	GraphTreeWindow* _window;

}; // struct GraphTreeView

} // namespace gui
} // namespace ingen

#endif // INGEN_GUI_GRAPHTREEWINDOW_HPP
