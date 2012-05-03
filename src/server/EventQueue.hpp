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

#ifndef INGEN_ENGINE_EVENTQUEUE_HPP
#define INGEN_ENGINE_EVENTQUEUE_HPP

#include "raul/AtomicPtr.hpp"
#include "raul/Slave.hpp"

#include "EventSink.hpp"
#include "EventSource.hpp"

namespace Ingen {
namespace Server {

class Event;
class PostProcessor;
class ProcessContext;

/** An EventSource which prepares events in its own thread.
 */
class EventQueue : public EventSource
                 , public EventSink
                 , public Raul::Slave
{
public:
	explicit EventQueue();
	virtual ~EventQueue();

	bool process(PostProcessor& dest, ProcessContext& context, bool limit=true);

	inline bool unprepared_events() const { return _prepared_back.get(); }
	inline bool empty()             const { return !_head.get(); }

protected:
	void event(Event* ev);

	virtual void _whipped(); ///< Prepare 1 event

private:
	Raul::AtomicPtr<Event> _head;
	Raul::AtomicPtr<Event> _prepared_back;
	Raul::AtomicPtr<Event> _tail;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_EVENTQUEUE_HPP

