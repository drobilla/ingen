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

#ifndef EVENTSOURCE_H
#define EVENTSOURCE_H

namespace Om {

class Event;
class QueuedEvent;


/** Source for events to run in the audio thread.
 *
 * The AudioDriver gets events from an EventSource in the process callback
 * (realtime audio thread) and executes them, then they are sent to the
 * PostProcessor and finalised (post-processing thread).
 */
class EventSource
{
public:

	virtual ~EventSource() {}

	virtual Event* pop_earliest_event_before(const samplecount time) = 0;

	virtual void start() = 0;
	
	virtual void stop() = 0;

protected:
	EventSource() {}
};


} // namespace Om

#endif // EVENTSOURCE_H

