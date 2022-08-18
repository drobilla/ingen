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

#ifndef INGEN_GUI_ARC_HPP
#define INGEN_GUI_ARC_HPP

#include "ganv/Edge.hpp"

#include <memory>

namespace Ganv {
class Canvas;
class Node;
} // namespace Ganv

namespace ingen {

namespace client {
class ArcModel;
} // namespace client

namespace gui {

/** An Arc (directed edge) in a Graph.
 *
 * \ingroup GUI
 */
class Arc : public Ganv::Edge
{
public:
	Arc(Ganv::Canvas&                                  canvas,
	    const std::shared_ptr<const client::ArcModel>& model,
	    Ganv::Node*                                    src,
	    Ganv::Node*                                    dst);

	std::shared_ptr<const client::ArcModel> model() const { return _arc_model; }

private:
	std::shared_ptr<const client::ArcModel> _arc_model;
};

} // namespace gui
} // namespace ingen

#endif // INGEN_GUI_ARC_HPP
