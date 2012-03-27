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


#ifndef INGEN_EVENTS_PING_HPP
#define INGEN_EVENTS_PING_HPP

#include "Event.hpp"
#include "types.hpp"

namespace Ingen {
namespace Server {

class PortImpl;

namespace Events {

/** A ping that travels through the pre-processed event queue before responding
 * (useful for the order guarantee).
 *
 * \ingroup engine
 */
class Ping : public Event
{
public:
	Ping(Engine&     engine,
	     Interface*  client,
	     int32_t     id,
	     SampleCount timestamp)
		: Event(engine, client, id, timestamp)
	{}

	void post_process() { respond(SUCCESS); }
};

} // namespace Server
} // namespace Ingen
} // namespace Events

#endif // INGEN_EVENTS_PING_HPP
