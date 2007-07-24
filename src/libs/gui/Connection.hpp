/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#ifndef CONNECTION_H
#define CONNECTION_H

#include <cassert>
#include <string>
#include <flowcanvas/Connection.hpp>
#include <raul/SharedPtr.hpp>
#include "client/ConnectionModel.hpp"
using Ingen::Client::ConnectionModel;

namespace Ingen {
namespace GUI {


/** A Connection in a Patch.
 * 
 * \ingroup GUI
 */
class Connection : public FlowCanvas::Connection
{
public:
	Connection(boost::shared_ptr<FlowCanvas::Canvas>  canvas,
	           boost::shared_ptr<ConnectionModel>            model,
	           boost::shared_ptr<FlowCanvas::Connectable> src,
	           boost::shared_ptr<FlowCanvas::Connectable> dst,
	           uint32_t                                      color)
	: FlowCanvas::Connection(canvas, src, dst, color)
	, _connection_model(model)
	{}

	virtual ~Connection() {}

	SharedPtr<ConnectionModel> model() const { return _connection_model; }
	
private:
	SharedPtr<ConnectionModel> _connection_model;
};


} // namespace GUI
} // namespace Ingen

#endif // CONNECTION_H
