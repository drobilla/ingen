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

#include "WindowFactory.hpp"

#include "App.hpp"
#include "GraphBox.hpp"
#include "GraphWindow.hpp"
#include "LoadGraphWindow.hpp"
#include "LoadPluginWindow.hpp"
#include "NewSubgraphWindow.hpp"
#include "PropertiesWindow.hpp"
#include "RenameWindow.hpp"
#include "WidgetFactory.hpp"

#include "ingen/Log.hpp"
#include "ingen/client/BlockModel.hpp"
#include "ingen/client/GraphModel.hpp"
#include "ingen/client/ObjectModel.hpp"
#include "raul/Path.hpp"

#include <gdkmm/window.h>
#include <sigc++/adaptors/bind.h>
#include <sigc++/functors/mem_fun.h>

#include <cassert>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace ingen {

using client::BlockModel;
using client::GraphModel;
using client::ObjectModel;

namespace gui {

WindowFactory::WindowFactory(App& app)
	: _app(app)
{
	WidgetFactory::get_widget_derived("load_plugin_win", _load_plugin_win);
	WidgetFactory::get_widget_derived("load_graph_win", _load_graph_win);
	WidgetFactory::get_widget_derived("new_subgraph_win", _new_subgraph_win);
	WidgetFactory::get_widget_derived("properties_win", _properties_win);
	WidgetFactory::get_widget_derived("rename_win", _rename_win);

	if (!(_load_plugin_win && _load_graph_win && _new_subgraph_win
	      && _properties_win && _rename_win)) {
		throw std::runtime_error("failed to load window widgets\n");
	}

	_load_plugin_win->init_window(app);
	_load_graph_win->init(app);
	_new_subgraph_win->init_window(app);
	_properties_win->init_window(app);
	_rename_win->init_window(app);
}

WindowFactory::~WindowFactory()
{
	for (const auto& w : _graph_windows) {
		delete w.second;
	}
}

void
WindowFactory::clear()
{
	for (const auto& w : _graph_windows) {
		delete w.second;
	}

	_graph_windows.clear();
}

/** Returns the number of Graph windows currently visible.
 */
size_t
WindowFactory::num_open_graph_windows()
{
	size_t ret = 0;
	for (const auto& w : _graph_windows) {
		if (w.second->is_visible()) {
			++ret;
		}
	}

	return ret;
}

GraphBox*
WindowFactory::graph_box(const std::shared_ptr<const GraphModel>& graph)
{
	GraphWindow* window = graph_window(graph);
	if (window) {
		return window->box();
	}

	return _main_box;
}

GraphWindow*
WindowFactory::graph_window(const std::shared_ptr<const GraphModel>& graph)
{
	if (!graph) {
		return nullptr;
	}

	auto w = _graph_windows.find(graph->path());

	return (w == _graph_windows.end()) ? nullptr : w->second;
}

GraphWindow*
WindowFactory::parent_graph_window(
    const std::shared_ptr<const BlockModel>& block)
{
	if (!block) {
		return nullptr;
	}

	return graph_window(std::dynamic_pointer_cast<GraphModel>(block->parent()));
}

/** Present a GraphWindow for a Graph.
 *
 * If `preferred` is not null, it will be set to display `graph` if the graph
 * does not already have a visible window, otherwise that window will be
 * presented and `preferred` left unmodified.
 */
void
WindowFactory::present_graph(const std::shared_ptr<const GraphModel>& graph,
                             GraphWindow*                             preferred,
                             const std::shared_ptr<GraphView>&        view)
{
	auto w = _graph_windows.find(graph->path());

	if (w != _graph_windows.end()) {
		(*w).second->present();
	} else if (preferred) {
		w = _graph_windows.find(preferred->graph()->path());
		assert((*w).second == preferred);

		preferred->box()->set_graph(graph, view);
		_graph_windows.erase(w);
		_graph_windows[graph->path()] = preferred;
		preferred->present();

	} else {
		GraphWindow* win = new_graph_window(graph, view);
		win->present();
	}
}

GraphWindow*
WindowFactory::new_graph_window(const std::shared_ptr<const GraphModel>& graph,
                                const std::shared_ptr<GraphView>&        view)
{
	GraphWindow* win = nullptr;
	WidgetFactory::get_widget_derived("graph_win", win);
	if (!win) {
		_app.log().error("Failed to load graph window widget\n");
		return nullptr;
	}

	win->init_window(_app);

	win->box()->set_graph(graph, view);
	_graph_windows[graph->path()] = win;

	win->signal_delete_event().connect(
		sigc::bind<0>(sigc::mem_fun(this, &WindowFactory::remove_graph_window),
		              win));

	return win;
}

bool
WindowFactory::remove_graph_window(GraphWindow* win, GdkEventAny* ignored)
{
	if (_graph_windows.size() <= 1) {
		return !_app.quit(win);
	}

	auto w = _graph_windows.find(win->graph()->path());

	assert((*w).second == win);
	_graph_windows.erase(w);

	delete win;

	return false;
}

void
WindowFactory::present_load_plugin(
    const std::shared_ptr<const GraphModel>& graph,
    const Properties&                        data)
{
	_app.request_plugins_if_necessary();

	auto w = _graph_windows.find(graph->path());

	if (w != _graph_windows.end()) {
		_load_plugin_win->set_transient_for(*w->second);
	}

	_load_plugin_win->set_modal(false);
	_load_plugin_win->set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
	if (w->second) {
		int width  = 0;
		int height = 0;
		w->second->get_size(width, height);
		_load_plugin_win->set_default_size(width - width / 8, height / 2);
	}
	_load_plugin_win->set_title(
		std::string("Load Plugin - ") + graph->path() + " - Ingen");
	_load_plugin_win->present(graph, data);
}

void
WindowFactory::present_load_graph(
    const std::shared_ptr<const GraphModel>& graph,
    const Properties&                        data)
{
	auto w = _graph_windows.find(graph->path());

	if (w != _graph_windows.end()) {
		_load_graph_win->set_transient_for(*w->second);
	}

	_load_graph_win->present(graph, true, data);
}

void
WindowFactory::present_load_subgraph(
    const std::shared_ptr<const GraphModel>& graph,
    const Properties&                        data)
{
	auto w = _graph_windows.find(graph->path());

	if (w != _graph_windows.end()) {
		_load_graph_win->set_transient_for(*w->second);
	}

	_load_graph_win->present(graph, false, data);
}

void
WindowFactory::present_new_subgraph(
    const std::shared_ptr<const GraphModel>& graph,
    const Properties&                        data)
{
	auto w = _graph_windows.find(graph->path());

	if (w != _graph_windows.end()) {
		_new_subgraph_win->set_transient_for(*w->second);
	}

	_new_subgraph_win->present(graph, data);
}

void
WindowFactory::present_rename(const std::shared_ptr<const ObjectModel>& object)
{
	auto w = _graph_windows.find(object->path());
	if (w == _graph_windows.end()) {
		w = _graph_windows.find(object->path().parent());
	}

	if (w != _graph_windows.end()) {
		_rename_win->set_transient_for(*w->second);
	}

	_rename_win->present(object);
}

void
WindowFactory::present_properties(
    const std::shared_ptr<const ObjectModel>& object)
{
	auto w = _graph_windows.find(object->path());
	if (w == _graph_windows.end()) {
		w = _graph_windows.find(object->path().parent());
	}
	if (w == _graph_windows.end()) {
		w = _graph_windows.find(object->path().parent().parent());
	}

	if (w != _graph_windows.end()) {
		_properties_win->set_transient_for(*w->second);
	}

	_properties_win->present(object);
}

} // namespace gui
} // namespace ingen
