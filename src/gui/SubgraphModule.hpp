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

#ifndef INGEN_GUI_SUBGRAPHMODULE_HPP
#define INGEN_GUI_SUBGRAPHMODULE_HPP

#include "ingen/types.hpp"

#include "NodeModule.hpp"
#include "GraphPortModule.hpp"

namespace Ingen { namespace Client {
class GraphModel;
class GraphWindow;
class PortModel;
} }

namespace Ingen {
namespace GUI {

class GraphCanvas;

/** A module to represent a subgraph
 *
 * \ingroup GUI
 */
class SubgraphModule : public NodeModule
{
public:
	SubgraphModule(GraphCanvas&                        canvas,
	               SPtr<const Client::GraphModel> graph);

	virtual ~SubgraphModule() {}

	bool on_double_click(GdkEventButton* event);

	void store_location(double ax, double ay);

	void browse_to_graph();
	void menu_remove();

	SPtr<const Client::GraphModel> graph() const { return _graph; }

protected:
	SPtr<const Client::GraphModel> _graph;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_SUBGRAPHMODULE_HPP
