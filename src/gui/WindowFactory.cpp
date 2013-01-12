/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <string>

#include "ingen/client/GraphModel.hpp"

#include "App.hpp"
#include "LoadGraphWindow.hpp"
#include "LoadPluginWindow.hpp"
#include "NewSubgraphWindow.hpp"
#include "GraphView.hpp"
#include "GraphWindow.hpp"
#include "PropertiesWindow.hpp"
#include "RenameWindow.hpp"
#include "WidgetFactory.hpp"
#include "WindowFactory.hpp"

using namespace std;

namespace Ingen {

using namespace Client;

namespace GUI {

WindowFactory::WindowFactory(App& app)
	: _app(app)
	, _main_box(NULL)
	, _load_plugin_win(NULL)
	, _load_graph_win(NULL)
	, _new_subgraph_win(NULL)
	, _properties_win(NULL)
{
	WidgetFactory::get_widget_derived("load_plugin_win", _load_plugin_win);
	WidgetFactory::get_widget_derived("load_graph_win", _load_graph_win);
	WidgetFactory::get_widget_derived("new_subgraph_win", _new_subgraph_win);
	WidgetFactory::get_widget_derived("properties_win", _properties_win);
	WidgetFactory::get_widget_derived("rename_win", _rename_win);

	_load_plugin_win->init_window(app);
	_load_graph_win->init(app);
	_new_subgraph_win->init_window(app);
	_properties_win->init_window(app);
	_rename_win->init_window(app);
}

WindowFactory::~WindowFactory()
{
	for (const auto& w : _graph_windows)
		delete w.second;
}

void
WindowFactory::clear()
{
	for (const auto& w : _graph_windows)
		delete w.second;

	_graph_windows.clear();
}

/** Returns the number of Graph windows currently visible.
 */
size_t
WindowFactory::num_open_graph_windows()
{
	size_t ret = 0;
	for (const auto& w : _graph_windows)
		if (w.second->is_visible())
			++ret;

	return ret;
}

GraphBox*
WindowFactory::graph_box(SPtr<const GraphModel> graph)
{
	GraphWindow* window = graph_window(graph);
	if (window) {
		return window->box();
	} else {
		return _main_box;
	}
}

GraphWindow*
WindowFactory::graph_window(SPtr<const GraphModel> graph)
{
	if (!graph)
		return NULL;

	GraphWindowMap::iterator w = _graph_windows.find(graph->path());

	return (w == _graph_windows.end()) ? NULL : w->second;
}

GraphWindow*
WindowFactory::parent_graph_window(SPtr<const BlockModel> block)
{
	if (!block)
		return NULL;

	return graph_window(dynamic_ptr_cast<GraphModel>(block->parent()));
}

/** Present a GraphWindow for a Graph.
 *
 * If @a preferred is not NULL, it will be set to display @a graph if the graph
 * does not already have a visible window, otherwise that window will be
 * presented and @a preferred left unmodified.
 */
void
WindowFactory::present_graph(SPtr<const GraphModel> graph,
                             GraphWindow*           preferred,
                             SPtr<GraphView>        view)
{
	assert(!view || view->graph() == graph);

	GraphWindowMap::iterator w = _graph_windows.find(graph->path());

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
WindowFactory::new_graph_window(SPtr<const GraphModel> graph,
                                SPtr<GraphView>        view)
{
	assert(!view || view->graph() == graph);

	GraphWindow* win = NULL;
	WidgetFactory::get_widget_derived("graph_win", win);
	win->init_window(_app);

	win->box()->set_graph(graph, view);
	_graph_windows[graph->path()] = win;

	win->signal_delete_event().connect(sigc::bind<0>(
		sigc::mem_fun(this, &WindowFactory::remove_graph_window), win));

	return win;
}

bool
WindowFactory::remove_graph_window(GraphWindow* win, GdkEventAny* ignored)
{
	if (_graph_windows.size() <= 1)
		return !_app.quit(win);

	GraphWindowMap::iterator w = _graph_windows.find(win->graph()->path());

	assert((*w).second == win);
	_graph_windows.erase(w);

	delete win;

	return false;
}

void
WindowFactory::present_load_plugin(SPtr<const GraphModel> graph,
                                   Node::Properties       data)
{
	_app.request_plugins_if_necessary();

	GraphWindowMap::iterator w = _graph_windows.find(graph->path());

	if (w != _graph_windows.end())
		_load_plugin_win->set_transient_for(*w->second);

	_load_plugin_win->set_modal(false);
	_load_plugin_win->set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
	if (w->second) {
		int width, height;
		w->second->get_size(width, height);
		_load_plugin_win->set_default_size(width - width / 8, height / 2);
	}
	_load_plugin_win->set_title(
		string("Load Plugin - ") + graph->path() + " - Ingen");
	_load_plugin_win->present(graph, data);
}

void
WindowFactory::present_load_graph(SPtr<const GraphModel> graph,
                                  Node::Properties       data)
{
	GraphWindowMap::iterator w = _graph_windows.find(graph->path());

	if (w != _graph_windows.end())
		_load_graph_win->set_transient_for(*w->second);

	_load_graph_win->present(graph, true, data);
}

void
WindowFactory::present_load_subgraph(SPtr<const GraphModel> graph,
                                     Node::Properties       data)
{
	GraphWindowMap::iterator w = _graph_windows.find(graph->path());

	if (w != _graph_windows.end())
		_load_graph_win->set_transient_for(*w->second);

	_load_graph_win->present(graph, false, data);
}

void
WindowFactory::present_new_subgraph(SPtr<const GraphModel> graph,
                                    Node::Properties       data)
{
	GraphWindowMap::iterator w = _graph_windows.find(graph->path());

	if (w != _graph_windows.end())
		_new_subgraph_win->set_transient_for(*w->second);

	_new_subgraph_win->present(graph, data);
}

void
WindowFactory::present_rename(SPtr<const ObjectModel> object)
{
	GraphWindowMap::iterator w = _graph_windows.find(object->path());
	if (w == _graph_windows.end())
		w = _graph_windows.find(object->path().parent());

	if (w != _graph_windows.end())
		_rename_win->set_transient_for(*w->second);

	_rename_win->present(object);
}

void
WindowFactory::present_properties(SPtr<const ObjectModel> object)
{
	GraphWindowMap::iterator w = _graph_windows.find(object->path());
	if (w == _graph_windows.end())
		w = _graph_windows.find(object->path().parent());
	if (w == _graph_windows.end())
		w = _graph_windows.find(object->path().parent().parent());

	if (w != _graph_windows.end())
		_properties_win->set_transient_for(*w->second);

	_properties_win->present(object);
}

} // namespace GUI
} // namespace Ingen
