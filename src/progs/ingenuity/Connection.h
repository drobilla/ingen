/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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
#include <flowcanvas/Connection.h>
#include "ConnectionModel.h"
#include "util/CountedPtr.h"
using Ingen::Client::ConnectionModel;

namespace Ingenuity {


/** A Connection on an Module.
 * 
 * \ingroup Ingenuity
 */
class Connection : public LibFlowCanvas::Connection
{
public:
	Connection(boost::shared_ptr<LibFlowCanvas::FlowCanvas> canvas,
	           boost::shared_ptr<ConnectionModel>           model,
	           boost::shared_ptr<LibFlowCanvas::Port>       src,
	           boost::shared_ptr<LibFlowCanvas::Port>       dst)
	: LibFlowCanvas::Connection(canvas, src, dst)
	, m_connection_model(model)
	{}

	virtual ~Connection() {}

	CountedPtr<ConnectionModel> model() const { return m_connection_model; }
	
private:
	CountedPtr<ConnectionModel> m_connection_model;
};


} // namespace Ingenuity

#endif // CONNECTION_H
