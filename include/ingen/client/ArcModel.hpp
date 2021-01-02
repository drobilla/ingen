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

#ifndef INGEN_CLIENT_ARCMODEL_HPP
#define INGEN_CLIENT_ARCMODEL_HPP

#include "ingen/Arc.hpp"
#include "ingen/client/PortModel.hpp"
#include "ingen/ingen.h"
#include "raul/Path.hpp"

#include <cassert>
#include <memory>
#include <string>
#include <utility>

namespace ingen {
namespace client {

/** Class to represent a port->port connections in the engine.
 *
 * @ingroup IngenClient
 */
class INGEN_API ArcModel : public Arc
{
public:
	std::shared_ptr<PortModel> tail() const { return _tail; }
	std::shared_ptr<PortModel> head() const { return _head; }

	const raul::Path& tail_path() const override { return _tail->path(); }
	const raul::Path& head_path() const override { return _head->path(); }

private:
	friend class ClientStore;

	ArcModel(std::shared_ptr<PortModel> tail, std::shared_ptr<PortModel> head)
		: _tail(std::move(tail))
		, _head(std::move(head))
	{
		assert(_tail);
		assert(_head);
		assert(_tail->parent());
		assert(_head->parent());
		assert(_tail->path() != _head->path());
	}

	const std::shared_ptr<PortModel> _tail;
	const std::shared_ptr<PortModel> _head;
};

} // namespace client
} // namespace ingen

#endif // INGEN_CLIENT_ARCMODEL_HPP
