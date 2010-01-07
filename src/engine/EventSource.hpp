/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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

#ifndef INGEN_EVENT_SOURCE_HPP
#define INGEN_EVENT_SOURCE_HPP

#include "raul/Semaphore.hpp"
#include "raul/Slave.hpp"
#include "raul/List.hpp"

namespace Ingen {

class Event;
class QueuedEvent;
class PostProcessor;
class ProcessContext;


/** Source for events to run in the audio thread.
 *
 * The Driver gets events from an EventSource in the process callback
 * (realtime audio thread) and executes them, then they are sent to the
 * PostProcessor and finalised (post-processing thread).
 */
class EventSource : protected Raul::Slave
{
public:
	EventSource(size_t queue_size);
	~EventSource();

	void activate_source()   { Slave::start(); }
	void deactivate_source() { Slave::stop(); }

	void process(PostProcessor& dest, ProcessContext& context);

	/** Signal that a blocking event is finished.
	 *
	 * This MUST be called by blocking events in their post_process() method
	 * to resume pre-processing of events.
	 */
	inline void unblock() { _blocking_semaphore.post(); }

protected:
	void push_queued(QueuedEvent* const ev);

	inline bool unprepared_events() { return (_prepared_back.get() != NULL); }

	virtual void _whipped(); ///< Prepare 1 event

private:
	Raul::List<Event*>                        _events;
	Raul::AtomicPtr<Raul::List<Event*>::Node> _prepared_back;
	Raul::Semaphore                           _blocking_semaphore;
};


} // namespace Ingen

#endif // INGEN_EVENT_SOURCE_HPP

