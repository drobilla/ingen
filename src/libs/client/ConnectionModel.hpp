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

#ifndef CONNECTIONMODEL_H
#define CONNECTIONMODEL_H

#include <cassert>
#include <string>
#include <list>
#include <raul/Path.hpp>
#include <raul/SharedPtr.hpp>
#include "interface/Connection.hpp"
#include "PortModel.hpp"

namespace Ingen {
namespace Client {

class Store;


/** Class to represent a port->port connection in the engine.
 *
 * This can either have pointers to the connection ports' models, or just
 * paths as strings.  The engine passes just strings (by necessity), but
 * clients can set the pointers then they don't have to worry about port
 * renaming, as the connections will always return the port's path, even
 * if it changes.
 *
 * \ingroup IngenClient
 */
class ConnectionModel : public Shared::Connection
{
public:
	SharedPtr<PortModel> src_port() const { return _src_port; }
	SharedPtr<PortModel> dst_port() const { return _dst_port; }

	const Path src_port_path() const { return _src_port->path(); }
	const Path dst_port_path() const { return _dst_port->path(); }
	
private:
	friend class Store;

	ConnectionModel(SharedPtr<PortModel> src, SharedPtr<PortModel> dst)
		: _src_port(src)
		, _dst_port(dst)
	{
		assert(_src_port);
		assert(_dst_port);
		assert(_src_port->parent());
		assert(_dst_port->parent());
	}
	
	SharedPtr<PortModel> _src_port;
	SharedPtr<PortModel> _dst_port;
};


} // namespace Client
} // namespace Ingen

#endif // CONNECTIONMODEL_H
