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

#ifndef INGEN_EVENTS_DEACTIVATE_HPP
#define INGEN_EVENTS_DEACTIVATE_HPP

#include "Event.hpp"
#include "Engine.hpp"

namespace Ingen {
namespace Server {
namespace Events {

/** Deactivates the engine.
 *
 * \ingroup engine
 */
class Deactivate : public Event
{
public:
	Deactivate(Engine&          engine,
	           ClientInterface* client,
	           int32_t          id,
	           SampleCount      timestamp)
		: Event(engine, client, id, timestamp)
	{}

	void post_process() {
		respond_ok();
		_engine.deactivate();
	}
};

} // namespace Server
} // namespace Ingen
} // namespace Events

#endif // INGEN_EVENTS_DEACTIVATE_HPP
