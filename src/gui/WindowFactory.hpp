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

#ifndef INGEN_GUI_WINDOWFACTORY_HPP
#define INGEN_GUI_WINDOWFACTORY_HPP

#include <map>

#include "ingen/GraphObject.hpp"
#include "raul/SharedPtr.hpp"

namespace Ingen {

namespace Client {
class BlockModel;
class ObjectModel;
class GraphModel;
}

namespace GUI {

class App;
class GraphBox;
class GraphView;
class GraphWindow;
class LoadGraphWindow;
class LoadPluginWindow;
class NewSubgraphWindow;
class PropertiesWindow;
class RenameWindow;

/** Manager/Factory for all windows.
 *
 * This serves as a nice centralized spot for all window management issues,
 * as well as an enumeration of all windows (the goal being to reduce that
 * number as much as possible).
 */
class WindowFactory {
public:
	explicit WindowFactory(App& app);
	~WindowFactory();

	size_t num_open_graph_windows();

	GraphBox*    graph_box(SharedPtr<const Client::GraphModel> graph);
	GraphWindow* graph_window(SharedPtr<const Client::GraphModel> graph);
	GraphWindow* parent_graph_window(SharedPtr<const Client::BlockModel> block);

	void present_graph(
		SharedPtr<const Client::GraphModel> model,
		GraphWindow*                        preferred = NULL,
		SharedPtr<GraphView>                view      = SharedPtr<GraphView>());

	typedef GraphObject::Properties Properties;

	void present_load_plugin(SharedPtr<const Client::GraphModel> graph, Properties data=Properties());
	void present_load_graph(SharedPtr<const Client::GraphModel> graph, Properties data=Properties());
	void present_load_subgraph(SharedPtr<const Client::GraphModel> graph, Properties data=Properties());
	void present_new_subgraph(SharedPtr<const Client::GraphModel> graph, Properties data=Properties());
	void present_rename(SharedPtr<const Client::ObjectModel> object);
	void present_properties(SharedPtr<const Client::ObjectModel> object);

	bool remove_graph_window(GraphWindow* win, GdkEventAny* ignored = NULL);

	void set_main_box(GraphBox* box) { _main_box = box; }

	void clear();

private:
	typedef std::map<Raul::Path, GraphWindow*> GraphWindowMap;

	GraphWindow* new_graph_window(SharedPtr<const Client::GraphModel> graph,
	                              SharedPtr<GraphView>                view);

	App&               _app;
	GraphBox*          _main_box;
	GraphWindowMap     _graph_windows;
	LoadPluginWindow*  _load_plugin_win;
	LoadGraphWindow*   _load_graph_win;
	NewSubgraphWindow* _new_subgraph_win;
	PropertiesWindow*  _properties_win;
	RenameWindow*      _rename_win;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_WINDOWFACTORY_HPP
