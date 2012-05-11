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

#ifndef INGEN_CLIENT_EDGEMODEL_HPP
#define INGEN_CLIENT_EDGEMODEL_HPP

#include <cassert>

#include "raul/Path.hpp"
#include "raul/SharedPtr.hpp"

#include "ingen/Edge.hpp"
#include "ingen/client/PortModel.hpp"

namespace Ingen {
namespace Client {

class ClientStore;

/** Class to represent a port->port connections in the engine.
 *
 * @ingroup IngenClient
 */
class EdgeModel : public Edge
{
public:
	SharedPtr<PortModel> tail() const { return _tail; }
	SharedPtr<PortModel> head() const { return _head; }

	const Raul::Path& tail_path() const { return _tail->path(); }
	const Raul::Path& head_path() const { return _head->path(); }

private:
	friend class ClientStore;

	EdgeModel(SharedPtr<PortModel> tail, SharedPtr<PortModel> head)
		: _tail(tail)
		, _head(head)
	{
		assert(_tail);
		assert(_head);
		assert(_tail->parent());
		assert(_head->parent());
		assert(_tail->path() != _head->path());
	}

	const SharedPtr<PortModel> _tail;
	const SharedPtr<PortModel> _head;
};

} // namespace Client
} // namespace Ingen

#endif // INGEN_CLIENT_EDGEMODEL_HPP
