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

#ifndef INGEN_GUI_WINDOWFACTORY_HPP
#define INGEN_GUI_WINDOWFACTORY_HPP

#include "ingen/Node.hpp"

#include <cstddef>
#include <map>
#include <memory>

namespace ingen {

namespace client {
class BlockModel;
class ObjectModel;
class GraphModel;
} // namespace client

namespace gui {

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

	GraphBox* graph_box(const std::shared_ptr<const client::GraphModel>& graph);

	GraphWindow*
	graph_window(const std::shared_ptr<const client::GraphModel>& graph);

	GraphWindow*
	parent_graph_window(const std::shared_ptr<const client::BlockModel>& block);

	void present_graph(const std::shared_ptr<const client::GraphModel>& graph,
	                   GraphWindow*                      preferred = nullptr,
	                   const std::shared_ptr<GraphView>& view      = nullptr);

	void
	present_load_plugin(const std::shared_ptr<const client::GraphModel>& graph,
	                    const Properties& data = Properties());

	void
	present_load_graph(const std::shared_ptr<const client::GraphModel>& graph,
	                   const Properties& data = Properties());

	void present_load_subgraph(
	    const std::shared_ptr<const client::GraphModel>& graph,
	    const Properties&                                data = Properties());

	void
	present_new_subgraph(const std::shared_ptr<const client::GraphModel>& graph,
	                     const Properties& data = Properties());

	void
	present_rename(const std::shared_ptr<const client::ObjectModel>& object);

	void present_properties(const std::shared_ptr<const client::ObjectModel>& object);

	bool remove_graph_window(GraphWindow* win, GdkEventAny* ignored = nullptr);

	void set_main_box(GraphBox* box) { _main_box = box; }

	void clear();

private:
	using GraphWindowMap = std::map<Raul::Path, GraphWindow*>;

	GraphWindow*
	new_graph_window(const std::shared_ptr<const client::GraphModel>& graph,
	                 const std::shared_ptr<GraphView>&                view);

	App&               _app;
	GraphBox*          _main_box;
	GraphWindowMap     _graph_windows;
	LoadPluginWindow*  _load_plugin_win;
	LoadGraphWindow*   _load_graph_win;
	NewSubgraphWindow* _new_subgraph_win;
	PropertiesWindow*  _properties_win;
	RenameWindow*      _rename_win;
};

} // namespace gui
} // namespace ingen

#endif // INGEN_GUI_WINDOWFACTORY_HPP
