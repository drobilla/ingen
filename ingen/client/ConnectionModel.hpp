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

#ifndef INGEN_CLIENT_CONNECTIONMODEL_HPP
#define INGEN_CLIENT_CONNECTIONMODEL_HPP

#include <cassert>

#include "raul/Path.hpp"
#include "raul/SharedPtr.hpp"

#include "ingen/Connection.hpp"
#include "ingen/client/PortModel.hpp"

namespace Ingen {
namespace Client {

class ClientStore;

/** Class to represent a port->port connections in the engine.
 *
 * \ingroup IngenClient
 */
class ConnectionModel : public Connection
{
public:
	SharedPtr<PortModel> src_port() const { return _src_port; }
	SharedPtr<PortModel> dst_port() const { return _dst_port; }

	const Raul::Path& src_port_path() const { return _src_port->path(); }
	const Raul::Path& dst_port_path() const { return _dst_port->path(); }

private:
	friend class ClientStore;

	ConnectionModel(SharedPtr<PortModel> src, SharedPtr<PortModel> dst)
		: _src_port(src)
		, _dst_port(dst)
	{
		assert(_src_port);
		assert(_dst_port);
		assert(_src_port->parent());
		assert(_dst_port->parent());
		assert(_src_port->path() != _dst_port->path());
	}

	const SharedPtr<PortModel> _src_port;
	const SharedPtr<PortModel> _dst_port;
};

} // namespace Client
} // namespace Ingen

#endif // INGEN_CLIENT_CONNECTIONMODEL_HPP
