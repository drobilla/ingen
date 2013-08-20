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

#ifndef INGEN_EVENTS_MOVE_HPP
#define INGEN_EVENTS_MOVE_HPP

#include "ingen/Store.hpp"
#include "raul/Path.hpp"

#include "Event.hpp"

namespace Ingen {
namespace Server {

class GraphImpl;
class PortImpl;

namespace Events {

/** \page methods
 * <h2>MOVE</h2>
 * As per WebDAV (RFC4918 S9.9).
 *
 * Move an object from its current location and insert it at a new location
 * in a single operation.
 *
 * MOVE to a path with a different parent is currently not supported.
 */

/** MOVE a graph object to a new path (see \ref methods).
 * \ingroup engine
 */
class Move : public Event
{
public:
	Move(Engine&           engine,
	     SPtr<Interface>   client,
	     int32_t           id,
	     SampleCount       timestamp,
	     const Raul::Path& old_path,
	     const Raul::Path& new_path);

	~Move();

	bool pre_process();
	void execute(ProcessContext& context);
	void post_process();

private:
	const Raul::Path _old_path;
	const Raul::Path _new_path;
};

} // namespace Events
} // namespace Server
} // namespace Ingen

#endif // INGEN_EVENTS_MOVE_HPP
