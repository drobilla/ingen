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

#include "App.hpp"
#include "GraphTreeWindow.hpp"
#include "Window.hpp"
#include "WindowFactory.hpp"

#include "ingen/Atom.hpp"
#include "ingen/Forge.hpp"
#include "ingen/Interface.hpp"
#include "ingen/Log.hpp"
#include "ingen/URIs.hpp"
#include "ingen/client/ClientStore.hpp"
#include "ingen/client/GraphModel.hpp"
#include "raul/Path.hpp"
#include "raul/Symbol.hpp"

#include <glibmm/propertyproxy.h>
#include <glibmm/signalproxy.h>
#include <gtkmm/builder.h>
#include <gtkmm/cellrenderer.h>
#include <gtkmm/cellrenderertoggle.h>
#include <gtkmm/object.h>
#include <gtkmm/treeiter.h>
#include <gtkmm/treepath.h>
#include <gtkmm/treeviewcolumn.h>
#include <sigc++/adaptors/bind.h>
#include <sigc++/functors/mem_fun.h>
#include <sigc++/signal.h>

#include <cassert>
#include <cstdint>
#include <memory>

namespace ingen {

using client::GraphModel;
using client::ObjectModel;

namespace gui {

GraphTreeWindow::GraphTreeWindow(BaseObjectType*                   cobject,
                                 const Glib::RefPtr<Gtk::Builder>& xml)
	: Window(cobject)
{
	xml->get_widget_derived("graphs_treeview", _graphs_treeview);

	_graph_treestore = Gtk::TreeStore::create(_graph_tree_columns);
	_graphs_treeview->set_window(this);
	_graphs_treeview->set_model(_graph_treestore);
	Gtk::TreeViewColumn* name_col = Gtk::manage(
		new Gtk::TreeViewColumn("Graph", _graph_tree_columns.name_col));
	Gtk::TreeViewColumn* enabled_col = Gtk::manage(
		new Gtk::TreeViewColumn("Run", _graph_tree_columns.enabled_col));
	name_col->set_resizable(true);
	name_col->set_expand(true);

	_graphs_treeview->append_column(*name_col);
	_graphs_treeview->append_column(*enabled_col);
	auto* enabled_renderer = dynamic_cast<Gtk::CellRendererToggle*>(
		_graphs_treeview->get_column_cell_renderer(1));
	enabled_renderer->property_activatable() = true;

	_graph_tree_selection = _graphs_treeview->get_selection();

	_graphs_treeview->signal_row_activated().connect(
		sigc::mem_fun(this, &GraphTreeWindow::event_graph_activated));
	enabled_renderer->signal_toggled().connect(
		sigc::mem_fun(this, &GraphTreeWindow::event_graph_enabled_toggled));

	_graphs_treeview->columns_autosize();
}

void
GraphTreeWindow::init(App& app, client::ClientStore& store)
{
	init_window(app);
	store.signal_new_object().connect(
		sigc::mem_fun(this, &GraphTreeWindow::new_object));
}

void
GraphTreeWindow::new_object(const std::shared_ptr<ObjectModel>& object)
{
	auto graph = std::dynamic_pointer_cast<GraphModel>(object);
	if (graph) {
		add_graph(graph);
	}
}

void
GraphTreeWindow::add_graph(const std::shared_ptr<GraphModel>& pm)
{
	if (!pm->parent()) {
		const auto iter = _graph_treestore->append();
		auto       row  = *iter;
		if (pm->path().is_root()) {
			row[_graph_tree_columns.name_col] = _app->interface()->uri().string();
		} else {
			row[_graph_tree_columns.name_col] = pm->symbol().c_str();
		}
		row[_graph_tree_columns.enabled_col] = pm->enabled();
		row[_graph_tree_columns.graph_model_col] = pm;
		_graphs_treeview->expand_row(_graph_treestore->get_path(iter), true);
	} else {
		const auto& children = _graph_treestore->children();
		auto        c        = find_graph(children, pm->parent());

		if (c != children.end()) {
			const auto iter = _graph_treestore->append(c->children());
			auto       row  = *iter;

			row[_graph_tree_columns.name_col] = pm->symbol().c_str();
			row[_graph_tree_columns.enabled_col] = pm->enabled();
			row[_graph_tree_columns.graph_model_col] = pm;
			_graphs_treeview->expand_row(_graph_treestore->get_path(iter), true);
		}
	}

	pm->signal_property().connect(
		sigc::bind(sigc::mem_fun(this, &GraphTreeWindow::graph_property_changed),
		           pm));

	pm->signal_moved().connect(
		sigc::bind(sigc::mem_fun(this, &GraphTreeWindow::graph_moved),
		           pm));

	pm->signal_destroyed().connect(
		sigc::bind(sigc::mem_fun(this, &GraphTreeWindow::remove_graph),
		           pm));
}

void
GraphTreeWindow::remove_graph(const std::shared_ptr<GraphModel>& pm)
{
	const auto i = find_graph(_graph_treestore->children(), pm);
	if (i != _graph_treestore->children().end()) {
		_graph_treestore->erase(i);
	}
}

Gtk::TreeModel::iterator
GraphTreeWindow::find_graph(Gtk::TreeModel::Children                    root,
                            const std::shared_ptr<client::ObjectModel>& graph)
{
	for (auto c = root.begin(); c != root.end(); ++c) {
		std::shared_ptr<GraphModel> pm = (*c)[_graph_tree_columns.graph_model_col];
		if (graph == pm) {
			return c;
		} else if (!(*c)->children().empty()) {
			auto ret = find_graph(c->children(), graph);
			if (ret != c->children().end()) {
				return ret;
			}
		}
	}
	return root.end();
}

/** Show the context menu for the selected graph in the graphs treeview.
 */
void
GraphTreeWindow::show_graph_menu(GdkEventButton* ev)
{
	const auto active = _graph_tree_selection->get_selected();
	if (active) {
		auto row = *active;
		auto col = _graph_tree_columns.graph_model_col;

		const std::shared_ptr<GraphModel>& pm = row[col];
		if (pm) {
			_app->log().warn("TODO: graph menu from tree window");
		}
	}
}

void
GraphTreeWindow::event_graph_activated(const Gtk::TreeModel::Path& path,
                                       Gtk::TreeView::Column*      col)
{
	const auto active = _graph_treestore->get_iter(path);
	auto       row    = *active;

	std::shared_ptr<GraphModel> pm = row[_graph_tree_columns.graph_model_col];

	_app->window_factory()->present_graph(pm);
}

void
GraphTreeWindow::event_graph_enabled_toggled(const Glib::ustring& path_str)
{
	Gtk::TreeModel::Path path(path_str);
	auto                 active = _graph_treestore->get_iter(path);
	auto                 row    = *active;

	std::shared_ptr<GraphModel> pm = row[_graph_tree_columns.graph_model_col];
	assert(pm);

	if (_enable_signal) {
		_app->set_property(pm->uri(),
		                   _app->uris().ingen_enabled,
		                   _app->forge().make(static_cast<bool>(!pm->enabled())));
	}
}

void
GraphTreeWindow::graph_property_changed(
    const URI&                         key,
    const Atom&                        value,
    const std::shared_ptr<GraphModel>& graph)
{
	const URIs& uris = _app->uris();
	_enable_signal = false;
	if (key == uris.ingen_enabled && value.type() == uris.forge.Bool) {
		const auto i = find_graph(_graph_treestore->children(), graph);
		if (i != _graph_treestore->children().end()) {
			auto row = *i;
			row[_graph_tree_columns.enabled_col] = value.get<int32_t>();
		} else {
			_app->log().error("Unable to find graph %1%\n", graph->path());
		}
	}
	_enable_signal = true;
}

void
GraphTreeWindow::graph_moved(const std::shared_ptr<GraphModel>& graph)
{
	_enable_signal = false;

	auto i = find_graph(_graph_treestore->children(), graph);
	if (i != _graph_treestore->children().end()) {
		auto row = *i;
		row[_graph_tree_columns.name_col] = graph->symbol().c_str();
	} else {
		_app->log().error("Unable to find graph %1%\n", graph->path());
	}

	_enable_signal = true;
}

} // namespace gui
} // namespace ingen
