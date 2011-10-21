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

#ifndef INGEN_ENGINE_EVENTSOURCE_HPP
#define INGEN_ENGINE_EVENTSOURCE_HPP

#include "raul/AtomicPtr.hpp"
#include "raul/Slave.hpp"

namespace Ingen {
namespace Server {

class Event;
class Event;
class PostProcessor;
class ProcessContext;

/** Source for events to run in the audio thread.
 *
 * The Driver gets events from an EventSource in the process callback
 * (realtime audio thread) and executes them, then they are sent to the
 * PostProcessor and finalised (post-processing thread).
 */
class EventSource : public Raul::Slave
{
public:
	explicit EventSource();
	virtual ~EventSource();

	void prepare_all();
	void process(PostProcessor& dest, ProcessContext& context, bool limit=true);

	inline bool unprepared_events() const { return (_prepared_back.get() != NULL); }
	inline bool empty()             const { return !_head.get(); }

protected:
	void push_queued(Event* const ev);

	virtual void _whipped(); ///< Prepare 1 event

private:
	Raul::AtomicPtr<Event> _head;
	Raul::AtomicPtr<Event> _prepared_back;
	Raul::AtomicPtr<Event> _tail;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_EVENTSOURCE_HPP

