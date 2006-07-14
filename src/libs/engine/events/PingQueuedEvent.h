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

#ifndef PINGQUEUEDEVENT_H
#define PINGQUEUEDEVENT_H

#include "QueuedEvent.h"
#include "types.h"
#include "Responder.h"

namespace Om {

class Port;


/** A ping that travels through the pre-processed event queue before responding
 * (useful for the order guarantee).
 *
 * \ingroup engine
 */
class PingQueuedEvent : public QueuedEvent
{
public:
	PingQueuedEvent(CountedPtr<Responder> responder, samplecount timestamp) : QueuedEvent(responder, timestamp) {}

	void post_process() { _responder->respond_ok(); }
};


} // namespace Om

#endif // PINGQUEUEDEVENT_H
