/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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

#include "client/ConnectionModel.hpp"
#include "Connection.hpp"
#include "Port.hpp"

using namespace std;

namespace Ingen {
namespace GUI {

Connection::Connection(boost::shared_ptr<FlowCanvas::Canvas>      canvas,
                       boost::shared_ptr<ConnectionModel>         model,
                       boost::shared_ptr<FlowCanvas::Connectable> src,
                       boost::shared_ptr<FlowCanvas::Connectable> dst,
                       uint32_t                                   color)
	: FlowCanvas::Connection(canvas, src, dst, color)
	, _connection_model(model)
{
	boost::shared_ptr<Port> src_port = boost::dynamic_pointer_cast<Port>(src);
	if (src_port)
		_bpath.property_dash() = src_port->dash();
}


}   // namespace GUI
}   // namespace Ingen
