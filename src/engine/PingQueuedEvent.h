/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef PINGQUEUEDEVENT_H
#define PINGQUEUEDEVENT_H

#include "QueuedEvent.h"
#include "util/types.h"
#include "Responder.h"

namespace Om {

class Port;


/** A "blocking" ping that travels through the event queue before responding.
 *
 * \ingroup engine
 */
class PingQueuedEvent : public QueuedEvent
{
public:
	PingQueuedEvent(CountedPtr<Responder> responder) : QueuedEvent(responder) {}

	void post_process() { m_responder->respond_ok(); }
};


} // namespace Om

#endif // PINGQUEUEDEVENT_H
