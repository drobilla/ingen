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

#include "SubgraphModule.hpp"

#include "App.hpp"
#include "NodeModule.hpp"
#include "WindowFactory.hpp"

#include "ingen/Atom.hpp"
#include "ingen/Forge.hpp"
#include "ingen/Interface.hpp"
#include "ingen/Resource.hpp"
#include "ingen/URIs.hpp"
#include "ingen/client/BlockModel.hpp"
#include "ingen/client/GraphModel.hpp"

#include <cassert>
#include <memory>

namespace ingen {

using client::GraphModel;

namespace gui {

class GraphWindow;

SubgraphModule::SubgraphModule(GraphCanvas&                             canvas,
                               const std::shared_ptr<const GraphModel>& graph)
    : NodeModule(canvas, graph), _graph(graph)
{
	assert(graph);
}

bool
SubgraphModule::on_double_click(GdkEventButton* event)
{
	assert(_graph);

	auto parent = std::dynamic_pointer_cast<GraphModel>(_graph->parent());

	GraphWindow* const preferred = ( (parent && (event->state & GDK_SHIFT_MASK))
	                                 ? nullptr
	                                 : app().window_factory()->graph_window(parent) );

	app().window_factory()->present_graph(_graph, preferred);
	return true;
}

void
SubgraphModule::store_location(double ax, double ay)
{
	const URIs& uris = app().uris();

	const Atom x(app().forge().make(static_cast<float>(ax)));
	const Atom y(app().forge().make(static_cast<float>(ay)));

	if (x != _block->get_property(uris.ingen_canvasX) ||
	    y != _block->get_property(uris.ingen_canvasY))
	{
		app().interface()->put(_graph->uri(),
		                       {{uris.ingen_canvasX, x},
		                        {uris.ingen_canvasY, y}},
		                       Resource::Graph::EXTERNAL);
	}
}

/** Browse to this graph in current (parent's) window
 * (unless an existing window is displaying it)
 */
void
SubgraphModule::browse_to_graph()
{
	assert(_graph->parent());

	auto parent = std::dynamic_pointer_cast<GraphModel>(_graph->parent());

	GraphWindow* const preferred = (parent)
		? app().window_factory()->graph_window(parent)
		: nullptr;

	app().window_factory()->present_graph(_graph, preferred);
}

void
SubgraphModule::menu_remove()
{
	app().interface()->del(_graph->uri());
}

} // namespace gui
} // namespace ingen
