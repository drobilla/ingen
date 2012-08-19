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

#include <cassert>
#include <utility>

#include "ingen/Interface.hpp"
#include "ingen/client/GraphModel.hpp"

#include "App.hpp"
#include "NodeModule.hpp"
#include "GraphCanvas.hpp"
#include "GraphWindow.hpp"
#include "Port.hpp"
#include "SubgraphModule.hpp"
#include "WindowFactory.hpp"

using namespace std;

namespace Ingen {

using namespace Client;

namespace GUI {

SubgraphModule::SubgraphModule(GraphCanvas&                canvas,
                               SharedPtr<const GraphModel> graph)
	: NodeModule(canvas, graph)
	, _graph(graph)
{
	assert(graph);
}

bool
SubgraphModule::on_double_click(GdkEventButton* event)
{
	assert(_graph);

	SharedPtr<GraphModel> parent = PtrCast<GraphModel>(_graph->parent());

	GraphWindow* const preferred = ( (parent && (event->state & GDK_SHIFT_MASK))
		? NULL
		: app().window_factory()->graph_window(parent) );

	app().window_factory()->present_graph(_graph, preferred);
	return true;
}

void
SubgraphModule::store_location(double ax, double ay)
{
	const URIs& uris = app().uris();

	const Raul::Atom x(app().forge().make(static_cast<float>(ax)));
	const Raul::Atom y(app().forge().make(static_cast<float>(ay)));

	if (x != _block->get_property(uris.ingen_canvasX) ||
	    y != _block->get_property(uris.ingen_canvasY))
	{
		Resource::Properties remove;
		remove.insert(make_pair(uris.ingen_canvasX, uris.wildcard));
		remove.insert(make_pair(uris.ingen_canvasY, uris.wildcard));
		Resource::Properties add;
		add.insert(make_pair(uris.ingen_canvasX,
		                     Resource::Property(x, Resource::EXTERNAL)));
		add.insert(make_pair(uris.ingen_canvasY,
		                     Resource::Property(y, Resource::EXTERNAL)));
		app().interface()->delta(_block->uri(), remove, add);
	}
}

/** Browse to this graph in current (parent's) window
 * (unless an existing window is displaying it)
 */
void
SubgraphModule::browse_to_graph()
{
	assert(_graph->parent());

	SharedPtr<GraphModel> parent = PtrCast<GraphModel>(_graph->parent());

	GraphWindow* const preferred = (parent)
		? app().window_factory()->graph_window(parent)
		: NULL;

	app().window_factory()->present_graph(_graph, preferred);
}

void
SubgraphModule::menu_remove()
{
	app().interface()->del(_graph->uri());
}

} // namespace GUI
} // namespace Ingen
