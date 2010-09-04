/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#ifndef INGEN_EVENTS_PING_HPP
#define INGEN_EVENTS_PING_HPP

#include "QueuedEvent.hpp"
#include "types.hpp"
#include "Request.hpp"

namespace Ingen {

class PortImpl;

namespace Events {


/** A ping that travels through the pre-processed event queue before responding
 * (useful for the order guarantee).
 *
 * \ingroup engine
 */
class Ping : public QueuedEvent
{
public:
	Ping(Engine& engine, SharedPtr<Request> request, SampleCount timestamp)
		: QueuedEvent(engine, request, timestamp)
	{}

	void post_process() { _request->respond_ok(); }
};


} // namespace Ingen
} // namespace Events

#endif // INGEN_EVENTS_PING_HPP
