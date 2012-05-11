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

#ifndef INGEN_GUI_EDGE_HPP
#define INGEN_GUI_EDGE_HPP

#include <cassert>
#include <string>

#include "ganv/Edge.hpp"
#include "raul/SharedPtr.hpp"

namespace Ingen {

namespace Client { class EdgeModel; }

namespace GUI {

/** An Edge in a Patch.
 *
 * \ingroup GUI
 */
class Edge : public Ganv::Edge
{
public:
	Edge(Ganv::Canvas&                              canvas,
	     boost::shared_ptr<const Client::EdgeModel> model,
	     Ganv::Node*                                src,
	     Ganv::Node*                                dst,
	     uint32_t                                   color);

	SharedPtr<const Client::EdgeModel> model() const { return _edge_model; }

private:
	SharedPtr<const Client::EdgeModel> _edge_model;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_EDGE_HPP
