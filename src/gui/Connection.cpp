/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
 *
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "Connection.hpp"
#include "Port.hpp"

#include "ingen/client/ConnectionModel.hpp"

using namespace std;

namespace Ingen {
namespace GUI {

Connection::Connection(Ganv::Canvas&                            canvas,
                       boost::shared_ptr<const ConnectionModel> model,
                       Ganv::Node*                              src,
                       Ganv::Node*                              dst,
                       uint32_t                                 color)
	: Ganv::Edge(canvas, src, dst, color)
	, _connection_model(model)
{
}

}   // namespace GUI
}   // namespace Ingen
